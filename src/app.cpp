#include "app.hpp"
#include "app_event.hpp"
#include "file_manager.hpp"
#include "scheduler.hpp"
#include <cstring>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>
#include <optional>
#include <print>
#include <string>
#include <wait.h>

namespace fs = std::filesystem;
namespace duck {
App::App(EventBus &event_bus, Ui &ui, FileManager &file_manager)
    : event_bus_{event_bus}, ui_{ui}, file_manager_{file_manager} {}

void App::run() {
  running_ = true;
  event_processing_thread_ = std::jthread([this] { process_events(); });
  event_bus_.push_event(FmgrEvent{
      .type_ = FmgrEvent::Type::LoadDirectory,
      .path = fs::current_path(),
  });
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
      std::visit(
          Visitor{
              [this](const FmgrEvent &event) { handle_fmgr_event(event); },
              [this](const RenderEvent &event) { handle_render_event(event); },
              [this](const DirectoryLoaded &event) {
                handle_directory_loaded(event);
              },
          },
          event);
    }
  }
}

void App::handle_directory_loaded(const DirectoryLoaded &event) {
  state_.cache_.insert(event.directory.path_, event.directory);
  state_.current_directory_ = event.directory;
  state_.current_path_ = event.directory.path_;
  state_.index_ = 0; // Reset index for new directory
  refresh_menu();
}

void App::handle_fmgr_event(const FmgrEvent &event) {
  switch (event.type_) {
  case FmgrEvent::Type::ToggleMark: {
    auto entry = state_.index_entry();
    if (entry) {
      if (state_.selected_entries_.contains(entry.value())) {
        state_.selected_entries_.erase(entry.value());
      } else {
        state_.selected_entries_.insert(entry.value());
      }
    }
    refresh_menu();
    break;
  }
  case FmgrEvent::Type::ToggleHidden:
    state_.show_hidden_ = !state_.show_hidden_;
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
  case RenderEvent::Type::MoveSelectionDown: {
    move_index_down();
    break;
  }
  case RenderEvent::Type::MoveSelectionUp: {
    move_index_up();
    break;
  }
  case RenderEvent::Type::EnterDirectory:
    enter_directory();
    break;
  case RenderEvent::Type::LeaveDirectory:
    leave_directory();
    break;
  case RenderEvent::Type::UpdatePreview:
    update_preview();
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

void App::move_index_down() {
  if (!state_.current_directory_.entries_.empty()) {
    state_.index_ =
        (state_.index_ + 1) % state_.current_directory_.entries_.size();
  }
  refresh_menu();
  update_preview();
}

void App::move_index_up() {
  if (!state_.current_directory_.entries_.empty()) {
    state_.index_ =
        (state_.index_ + state_.current_directory_.entries_.size() - 1) %
        state_.current_directory_.entries_.size();
  }
  refresh_menu();
}

stdexec::sender auto App::update_directory_preview_async() {
  const auto [width, height] = ftxui::Terminal::Size();
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           // ui_.post_task(
           //     [this] { state_.entries_preview = ftxui::text("Loading...");
           //     });
         }) |
         stdexec::then([this, height]() {
           return std::make_pair(state_.index_, height);
         }) |
         stdexec::then([this](const auto pair) {
           // FileManager::directory_preview(state_, pair);
         }) |
         stdexec::then(
             [this]() { ui_.post_task([this]() { ui_.render(state_); }); });
}

stdexec::sender auto App::update_text_preview_async() {
  auto [width, height] = ftxui::Terminal::Size();
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           // ui_.post_task([this]() { state_.text_preview = "Loading..."; });
         }) |
         stdexec::then([this, width, height]() {
           // return FileManager::text_preview(state_,
           // std::make_pair(width, height));
           return "";
         }) |
         stdexec::then([this](std::string preview) {
           ui_.post_task([this, prev = std::move(preview)]() {
             // state_.text_preview = prev;
             ui_.render(state_);
           });
         });
}

void App::refresh_menu() {
  ui_.update_info({state_.current_path_.string(), (int)state_.index_,
                   state_.current_directory_elements()});
}

void App::reload_menu() {
  event_bus_.push_event(FmgrEvent{.type_ = FmgrEvent::Type::Reload});
}

void App::update_preview() {
  // const auto selected_path = state_.index_entry();
  // if (selected_path) {
  //   if (fs::is_directory(selected_path.value())) {
  //     stdexec::sync_wait(update_directory_preview_async());
  //   } else {
  //     stdexec::sync_wait(update_text_preview_async());
  //   }
  // }
}

void App::enter_directory() {
  state_.index_entry().transform([this](const auto &entry) {
    if (entry.is_directory()) {
      if (auto cached = state_.cache_.get(entry.path()); cached) {
        handle_directory_loaded(DirectoryLoaded{cached.value()});
      } else {
        event_bus_.push_event(FmgrEvent{.type_ = FmgrEvent::Type::LoadDirectory,
                                        .path = entry.path()});
      }
    }
    return entry;
  });
}

void App::leave_directory() {
  auto parent_path = state_.current_path_.parent_path();
  if (auto cached = state_.cache_.get(parent_path); cached) {
    handle_directory_loaded(DirectoryLoaded{cached.value()});
  } else {
    event_bus_.push_event(FmgrEvent{.type_ = FmgrEvent::Type::LoadDirectory,
                                    .path = parent_path});
  }
}

void App::confirm_deletion() {
  std::vector<fs::path> paths;
  if (state_.selected_entries_.empty()) {
    if (auto entry = state_.index_entry()) {
      paths.push_back(entry->path());
    }
  } else {
    for (const auto &entry : state_.selected_entries_) {
      paths.push_back(entry.path());
    }
  }
  event_bus_.push_event(
      FmgrEvent{.type_ = FmgrEvent::Type::Deletion, .paths = paths});
  ui_.toggle_deletion_dialog();
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
  state_.index_entry().transform([this, new_name](const auto &entry) {
    event_bus_.push_event(
        FmgrEvent{.type_ = FmgrEvent::Type::Rename,
                  .path = entry.path(),
                  .path_to = state_.current_path_ / new_name});
    return entry;
  });
  ui_.toggle_rename_dialog();
}

void App::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"},       {".cpp", "nvim"},  {".c", "nvim"},
      {".md", "zen-browser"}, {".json", "nvim"}, {".gitignore", "nvim "}};

  try {
    auto selected_file_opt = state_.index_entry();
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
