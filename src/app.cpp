#include "app.hpp"
#include "app_event.hpp"
#include "colorscheme.hpp"
#include "file_manager.hpp"
#include "scheduler.hpp"
#include <cstring>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>
#include <print>
#include <ranges>
#include <string>
#include <wait.h>

namespace fs = std::filesystem;
namespace duck {
App::App(EventBus &event_bus, Ui &ui) : event_bus_{event_bus}, ui_{ui} {
  FileManagerService::init(state_);
}

void App::run() {
  event_processing_thread_ = std::jthread([this] { process_events(); });
  ui_.render();
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
          EventVisitor{
              [this](const FmgrEvent &event) { handle_fmgr_event(event); },
              [this](const RenderEvent &event) { handle_render_event(event); }},
          event);
    }
  }
}

void App::handle_fmgr_event(const FmgrEvent &event) {
  switch (event.type_) {
  case FmgrEvent::Type::OpenFile:
    open_file();
    break;
  case FmgrEvent::Type::ToggleMark:
    FileManagerService::toggle_mark_on_selected(state_);
    break;
  case FmgrEvent::Type::ToggleHidden:
    FileManagerService::toggle_hidden_entries(state_);
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
  case FmgrEvent::Type::StartYanking:
    FileManagerService::start_yanking(state_);
    break;
  case FmgrEvent::Type::StartCutting:
    FileManagerService::start_cutting(state_);
    break;
  case FmgrEvent::Type::Paste:
    FileManagerService::yank_or_cut(state_);
    break;
  }
  refresh_menu_async();
}

void App::handle_render_event(const RenderEvent &event) {
  switch (event.type_) {
  case RenderEvent::Type::MoveSelectionDown: {
    auto entries = FileManagerService::curdir_entries(state_);
    if (!entries.empty()) {
      state_.selected = (state_.selected + 1) % entries.size();
    }
    break;
  }
  case RenderEvent::Type::MoveSelectionUp: {
    auto entries = FileManagerService::curdir_entries(state_);
    if (!entries.empty()) {
      state_.selected = (state_.selected + entries.size() - 1) % entries.size();
    }
    break;
  }
  case RenderEvent::Type::EnterDirectory:
    enter_directory();
    break;
  case RenderEvent::Type::LeaveDirectory:
    leave_directory();
    break;
  case RenderEvent::Type::UpdatePreview:
    update_preview_async();
    break;
  case RenderEvent::Type::ShowNotification:
    ui_.toggle_notification();
    break;
  case RenderEvent::Type::HideNotification:
    ui_.toggle_notification();
    break;
  case RenderEvent::Type::ClearMarks:
    FileManagerService::clear_marked_entries(state_);
    break;
  case RenderEvent::Type::RefreshMenu:
    refresh_menu_async();
    break;
  case RenderEvent::Type::ReloadMenu:
    reload_menu_async();
    break;
  case RenderEvent::Type::Quit:
    ui_.exit();
    break;
  }
  ui_.post_task([this] { ui_.render(); });
}

stdexec::sender auto App::update_directory_preview_async() {
  const auto [width, height] = ftxui::Terminal::Size();
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           ui_.post_task(
               [this] { state_.entries_preview = ftxui::text("Loading..."); });
         }) |
         stdexec::then([this, height]() {
           return std::make_pair(state_.selected, height);
         }) |
         stdexec::then([this](const auto pair) {
           FileManagerService::directory_preview(state_, pair);
         }) |
         stdexec::then([this]() { ui_.post_task([this]() { ui_.render(); }); });
}

stdexec::sender auto App::update_text_preview_async() {
  auto [width, height] = ftxui::Terminal::Size();
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           ui_.post_task([this]() { state_.text_preview = "Loading..."; });
         }) |
         stdexec::then([this, width, height]() {
           return FileManagerService::text_preview(
               state_, std::make_pair(width, height));
         }) |
         stdexec::then([this](std::string preview) {
           ui_.post_task([this, prev = std::move(preview)]() {
             state_.text_preview = prev;
             ui_.render();
           });
         });
}

void App::refresh_menu_async() {
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([this]() {
                return FileManagerService::curdir_entries(state_);
              }) |
              stdexec::then([this](const auto &entries) {
                return entries_to_elements(entries);
              }) |
              stdexec::then([this](std::vector<ftxui::Element> elements) {
                ui_.post_task([this, elmt = std::move(elements)]() {
                  ui_.update_curdir_entries(state_, elmt);
                });
              });
  stdexec::sync_wait(task);
}

void App::reload_menu_async() {
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([this]() {
                return FileManagerService::update_curdir_entries(state_, false);
              }) |
              stdexec::then([this](const auto &entries) {
                return entries_to_elements(entries);
              }) |
              stdexec::then([this](std::vector<ftxui::Element> elements) {
                ui_.post_task([this, elmt = std::move(elements)]() {
                  ui_.update_curdir_entries(state_, elmt);
                });
              });
  stdexec::sync_wait(task);
}

void App::update_preview_async() {
  const auto selected_path = FileManagerService::selected_entry(state_);
  if (selected_path) {
    if (fs::is_directory(selected_path.value())) {
      stdexec::sync_wait(update_directory_preview_async());
    } else {
      stdexec::sync_wait(update_text_preview_async());
    }
  }
}

void App::enter_directory() {
  auto entry = FileManagerService::selected_entry(state_).and_then(
      [](fs::directory_entry entry) -> std::optional<fs::directory_entry> {
        if (fs::is_directory(entry)) {
          return std::make_optional(entry);
        }
        return std::nullopt;
      });

  if (entry) {
    auto task =
        stdexec::schedule(Scheduler::io_scheduler()) |
        stdexec::then([et = std::move(entry)]() { return et.value().path(); }) |
        stdexec::then([this](const fs::path &path) {
          FileManagerService::update_current_path(state_, path);
        }) |
        stdexec::then([this]() {
          return FileManagerService::update_curdir_entries(state_, true);
        }) |
        stdexec::then([this](const auto &entries) {
          return entries_to_elements(entries);
        }) |
        stdexec::then([this](std::vector<ftxui::Element> elements) {
          ui_.post_task([this, elem = std::move(elements)]() {
            ui_.enter_direcotry(state_, elem);
            update_preview_async();
          });
        });
    stdexec::sync_wait(task);
  }
}

void App::leave_directory() {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([this]() { return state_.parent_path; }) |
      stdexec::then([this](const fs::path &path) {
        FileManagerService::update_current_path(state_, path);
      }) |
      stdexec::then([this]() {
        return FileManagerService::update_curdir_entries(state_, true);
      }) |
      stdexec::then([this](const auto &entries) {
        return entries_to_elements(entries);
      }) |
      stdexec::then([this](std::vector<ftxui::Element> entries) {
        return std::make_pair(std::move(entries),
                              FileManagerService::previous_path_index(state_));
      }) |
      stdexec::then([this](std::pair<std::vector<ftxui::Element>, int> pair) {
        ui_.post_task([this, pair = std::move(pair)]() {
          ui_.leave_direcotry(state_, pair.first, pair.second);
        });
        return pair.second;
      }) |
      stdexec::let_value(
          [this](const int &) { return update_directory_preview_async(); });
  stdexec::sync_wait(task);
}

void App::confirm_deletion() {
  if (not state_.marked_entries.empty()) {
    auto task =
        stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
          return FileManagerService::delete_marked_entries(state_);
        }) |
        stdexec::then([this](bool success) {
          return FileManagerService::update_curdir_entries(state_, !success);
        }) |
        stdexec::then([this](const auto &entries) {
          return entries_to_elements(entries);
        }) |
        stdexec::then([this](std::vector<ftxui::Element> element) {
          ui_.post_task([this, elem = std::move(element)]() {
            ui_.update_curdir_entries(state_, elem);
            ui_.toggle_deletion_dialog();
          });
        });
    stdexec::sync_wait(task);
  } else {
    auto task =
        stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
          return FileManagerService::delete_selected_entry(state_);
        }) |
        stdexec::then([this](bool success) {
          return FileManagerService::update_curdir_entries(state_, !success);
        }) |
        stdexec::then([this](const auto &entries) {
          return entries_to_elements(entries);
        }) |
        stdexec::then([this](std::vector<ftxui::Element> element) {
          ui_.post_task([this, elem = std::move(element)]() {
            ui_.update_curdir_entries(state_, elem);
            ui_.toggle_deletion_dialog();
          });
        });
    stdexec::sync_wait(task);
  }
}

void App::confirm_creation() {
  auto filename = ui_.new_entry_input();
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([this, filename]() {
                FileManagerService::create_new_entry(state_, filename);
              }) |
              stdexec::then([this]() {
                return FileManagerService::update_curdir_entries(state_, false);
              }) |
              stdexec::then([this](const auto &entries) {
                return entries_to_elements(entries);
              }) |
              stdexec::then([this](std::vector<ftxui::Element> element) {
                ui_.post_task([this, elem = std::move(element)]() {
                  ui_.update_curdir_entries(state_, elem);
                  ui_.toggle_creation_dialog();
                });
              });
  stdexec::sync_wait(task);
}

void App::confirm_rename() {
  auto str = ui_.rename_input();
  auto rename_task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([this, str]() {
        FileManagerService::rename_selected_entry(state_, str);
      }) |
      stdexec::then([this]() {
        return FileManagerService::update_curdir_entries(state_, false);
      }) |
      stdexec::then([this](const auto &entries) {
        return entries_to_elements(entries);
      }) |
      stdexec::then([this](std::vector<ftxui::Element> element) {
        ui_.post_task([this, elem = std::move(element)]() {
          ui_.update_curdir_entries(state_, elem);
          ui_.toggle_rename_dialog();
        });
      });
  stdexec::sync_wait(rename_task);
}

void App::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"},       {".cpp", "nvim"},  {".c", "nvim"},
      {".md", "zen-browser"}, {".json", "nvim"}, {".gitignore", "nvim "}};

  try {
    auto selected_file_opt = FileManagerService::selected_entry(state_);
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

std::vector<ftxui::Element>
App::entries_to_elements(const std::vector<fs::directory_entry> &entries) {
  if (entries.empty()) {
    return {};
  }
  return entries |
         std::views::transform([this](const fs::directory_entry &entry) {
           auto filename = ftxui::text(entry_name_with_icon(entry));
           auto marker = ftxui::text("  ");
           if (FileManagerService::is_marked(state_, entry)) {
             marker = ftxui::text("█ ");
             if (state_.is_yanking) {
               marker = ftxui::text("█ ") | ftxui::color(ftxui::Color::Blue);
             } else if (state_.is_cutting) {
               marker = ftxui::text("█ ") | ftxui::color(ftxui::Color::Red);
             } else {
               marker = ftxui::text("█ ");
             }
           }
           auto elmt = ftxui::hbox({marker, filename});
           if (entry.is_directory()) {
             elmt |= ftxui::color(ColorScheme::dir());
           } else {
             elmt |= ftxui::color(ColorScheme::file());
           }
           return elmt;
         }) |
         std::ranges::to<std::vector>();
}

} // namespace duck
#include "stdexec/stop_token.hpp"
