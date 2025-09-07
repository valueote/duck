#include "app.hpp"
#include "app_event.hpp"
#include "file_manager.hpp"
#include "ftxui/dom/elements.hpp"
#include "utils.hpp"
#include <cstring>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>
#include <optional>
#include <print>
#include <stdexec/execution.hpp>
#include <string>
#include <wait.h>

namespace fs = std::filesystem;
namespace duck {
App::App(EventBus &event_bus, Ui &ui, FileManager &file_manager)
    : event_bus_{event_bus}, ui_{ui}, file_manager_{file_manager} {}

void App::run() {
  running_ = true;
  event_processing_thread_ = std::jthread([this] { process_events(); });
  state_.current_path_ = fs::current_path();
  state_.current_directory_ = FileManager::load_directory(state_.current_path_);
  state_.cache_.insert(state_.current_path_, state_.current_directory_);
  update_preview();
  ui_.render(state_);
}

void App::stop() {
  running_ = false;
  if (event_processing_thread_.joinable()) {
    event_processing_thread_.join();
  }
}

void App::process_events() {
  while (running_) {
    auto event_opt = event_bus_.pop_event();
    if (event_opt) {
      auto event = event_opt.value();
      std::visit(Visitor{[this](FmgrEvent &event) { handle_fmgr_event(event); },
                         [this](const RenderEvent &event) {
                           handle_render_event(event);
                         },
                         [this](const DirecotryLoaded &event) {
                           handle_directory_loaded(event);
                         },
                         [this](const PreviewUpdated &event) {
                           ui_.async_update_preview(event.preview_);
                         },
                         [this](const DirectoryPreviewRequested &event) {
                           handle_directory_preview_requested(event);
                         }},
                 event);
    }
  }
}

void App::handle_directory_loaded(const DirecotryLoaded &event) {
  state_.cache_.insert(event.directory_.path_, event.directory_);
}

void App::handle_fmgr_event(const FmgrEvent &event) {
  switch (event.type_) {
  case FmgrEvent::Type::UpdateCurrentDirectory: {
    update_current_direcotry(event.path);
    break;
  }
  case FmgrEvent::Type::ToggleSelection: {
    auto entry = state_.indexed_entry();
    if (entry) {
      if (state_.selected_entries_.contains(entry.value())) {
        state_.selected_entries_.erase(entry.value());
      } else {
        state_.selected_entries_.insert(entry.value());
      }
      move_index_down();
    }
    refresh_menu();
    break;
  }
  case FmgrEvent::Type::ToggleHidden:
    state_.toggle_hidden();
    refresh_menu();
    break;
  case FmgrEvent::Type::OpenFile:
    open_file();
    break;
  case FmgrEvent::Type::Deletion:
    confirm_deletion();
    break;
  case FmgrEvent::Type::Creation:
    confirm_creation();
    break;
  case FmgrEvent::Type::Rename:
    confirm_rename();
    break;
  case FmgrEvent::Type::RenameSuccess: {
    state_.rename_entry(event.path, event.path_to);
    refresh_menu();
    break;
  }
  case FmgrEvent::Type::Paste: {
    std::vector<fs::path> paths;
    paths.reserve(state_.selected_entries_.size());
    for (const auto &entry : state_.selected_entries_) {
      paths.push_back(entry.path());
    }
    event_bus_.push_event(FmgrEvent{
        .type_ = FmgrEvent::Type::Paste,
        .path = state_.current_path_,
        .paths = paths,
        .is_cutting = state_.is_cutting_,
    });
    state_.selected_entries_.clear();
    state_.is_cutting_ = false;
    state_.is_yanking_ = false;
    break;
  }
  default:
    file_manager_.handle_event(event);
    break;
  }
}

void App::handle_render_event(const RenderEvent &event) {
  switch (event.type_) {
  case RenderEvent::Type::MoveIndexDown: {
    move_index_down();
    break;
  }
  case RenderEvent::Type::MoveIndexUp: {
    move_index_up();
    break;
  }
  case RenderEvent::Type::EnterDirectory:
    enter_directory();
    break;
  case RenderEvent::Type::LeaveDirectory:
    leave_directory();
    break;
  case RenderEvent::Type::ToggleDeletionDialog:
    ui_.async_toggle_deletion_dialog();
    ui_.async_update_selected(ftxui::vbox(state_.selected_entries_elements()));
    break;
  case RenderEvent::Type::ToggleCreationDialog:
    ui_.toggle_creation_dialog();
    break;
  case RenderEvent::Type::ToggleRenameDialog:
    ui_.async_toggle_rename_dialog();
    ui_.update_rename_input(
        state_.indexed_entry().value().path().filename().string());
    break;
  case RenderEvent::Type::ToggleNotification:
    ui_.toggle_notification();
    break;
  case RenderEvent::Type::ClearMarks:
    state_.selected_entries_.clear();
    refresh_menu();
    break;
  case RenderEvent::Type::RefreshMenu:
    refresh_menu();
    break;
  case RenderEvent::Type::ReloadMenu:
    reload_menu();
    break;
  case RenderEvent::Type::Quit:
    running_ = false;
    ui_.exit();
    break;
  }
}

void App::handle_directory_preview_requested(
    const DirectoryPreviewRequested &event) {
  if (auto entries = state_.get_entries(event.path_); not entries.empty()) {
    ui_.async_update_preview(ftxui::vbox(state_.entries_to_elements(entries)));
  }
}

void App::update_current_direcotry(const fs::path &path) {
  auto directory = state_.cache_.get(path).value();
  state_.current_directory_ = directory;
  state_.current_path_ = directory.path_;
  state_.index_ = 0;
  refresh_menu();
  update_preview();
}

void App::move_index_down() {
  state_.move_index_down();
  ui_.async_update_index(state_.index_);
  update_preview();
}

void App::move_index_up() {
  state_.move_index_up();
  ui_.async_update_index(state_.index_);
  update_preview();
}

void App::refresh_menu() {
  ui_.async_update_info({state_.current_path_.string(), state_.index_,
                         state_.current_directory_elements()});
}

void App::reload_menu() {
  event_bus_.push_event(FmgrEvent{.type_ = FmgrEvent::Type::Reload});
}

void App::update_preview() {
  auto entry_opt = state_.indexed_entry();
  if (!entry_opt) {
    ui_.async_update_preview("[No item selected]");
    return;
  }

  const auto &entry = entry_opt.value();

  if (not entry.exists()) {
    ui_.async_update_preview("[File does not exists]");
    return;
  }

  if (auto entries = state_.get_entries(entry.path()); not entries.empty()) {
    ui_.async_update_preview(ftxui::vbox(state_.entries_to_elements(entries)));
    return;
  }

  auto [width, height] = ftxui::Terminal::Size();
  ui_.async_update_preview("Loading...");
  file_manager_.async_update_preview(entry, {width / 2, height - 4});
}

void App::enter_directory() {
  state_.indexed_entry().transform([this](const auto &entry) {
    if (entry.is_directory()) {
      if (state_.cache_.get(entry.path())) {
        update_current_direcotry(entry.path());
      } else {
        file_manager_.async_update_current_directory(entry.path());
      }
    }
    return entry;
  });
}

void App::leave_directory() {
  if (state_.current_path_ != state_.current_path_.root_path()) {
    auto parent_path = state_.current_path_.parent_path();
    if (state_.cache_.get(parent_path)) {
      update_current_direcotry(parent_path);
    } else {
      file_manager_.async_update_current_directory(parent_path);
    }
  }
}

// TODO: Set index after deletion

void App::confirm_deletion() {
  auto paths = state_.get_selected_entries();
  file_manager_.async_delete_entries(paths);
  state_.remove_entries(paths);
  ui_.async_toggle_deletion_dialog();
  refresh_menu();
  update_preview();
}

void App::confirm_creation() {
  auto filename = ui_.input_content();
  event_bus_.push_event(FmgrEvent{.type_ = FmgrEvent::Type::Creation,
                                  .path = state_.current_path_ / filename,
                                  .is_directory = filename.ends_with('/')});
  ui_.toggle_creation_dialog();
}

void App::confirm_rename() {
  auto new_name = ui_.input_content();
  if (auto entry = state_.indexed_entry(); entry) {
    auto old_path = entry.value().path();
    auto new_path = old_path.parent_path() / new_name;

    file_manager_.async_rename_entry(old_path, new_path);
    ui_.async_toggle_rename_dialog();
  }
}

void App::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"},       {".cpp", "nvim"},  {".c", "nvim"},
      {".md", "zen-browser"}, {".json", "nvim"}, {".gitignore", "nvim "}};

  try {
    auto selected_file_opt = state_.indexed_entry();
    if (!selected_file_opt.has_value()) {
      std::println(stderr, "[ERROR]: No file selected for opening");
      return;
    }

    std::string ext = selected_file_opt.value().path().extension().string();
    if (!handlers.contains(ext)) {
      std::println(stderr, "[WARNING]: No handler found for file extension: {}",
                   ext);
      return;
    }

    ui_.restored_io([&]() {
      pid_t pid = fork();
      if (pid == -1) {
        std::println(stderr, "[ERROR]: fork failed in open_file: {}",
                     strerror(errno));
        return;
      }

      if (pid == 0) {
        const char *handler = handlers.at(ext).c_str();
        const char *file_path = selected_file_opt->path().c_str();

        execlp(handler, handler, file_path, static_cast<char *>(nullptr));

        std::println(stderr, "[ERROR]: execlp failed for {}: {}", handler,
                     strerror(errno));
        _exit(EXIT_FAILURE);
      } else {
        int status{0};
        if (waitpid(pid, &status, 0) == -1) {
          std::println(stderr, "[ERROR]: waitpid failed: {}", strerror(errno));
          return;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
          std::println(stderr,
                       "[WARNING]: Child process exited with status: {}",
                       WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
          std::println(stderr,
                       "[WARNING]: Child process terminated by signal: {}",
                       WTERMSIG(status));
        }
      }
    });
  } catch (const std::exception &e) {
    std::println(stderr, "[ERROR]: Exception in open_file: {}", e.what());
  } catch (...) {
    std::println(stderr, "[ERROR]: Unknown exception in open_file");
  }
}

} // namespace duck
