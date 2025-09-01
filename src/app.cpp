#include "app.hpp"
#include "app_event.hpp"
#include "colorscheme.hpp"
#include "scheduler.hpp"
#include "stdexec/stop_token.hpp"
#include <cstring>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>
#include <print>
#include <ranges>
#include <string>

namespace fs = std::filesystem;
namespace duck {
App::App(EventBus &event_bus, Ui &ui, FileManager &file_manager)
    : event_bus_{event_bus}, ui_{ui}, file_manager_{file_manager} {}

void App::start_processing() {
  event_processing_thread_ = std::jthread([this](const std::stop_token &token) {
    while (!token.stop_requested() && running_) {
      auto event_opt = event_bus_.pop_event();
      if (event_opt) {
        auto event = event_opt.value();
        std::visit(EventVisitor{[](const FmgrEvent &event) {},
                                [](const RenderEvent &event) {}},
                   event);
      }
    }
  });
}

void App::stop_processing() {
  running_ = false;
  if (event_processing_thread_.joinable()) {
    event_processing_thread_.request_stop();
    event_processing_thread_.join();
  }
}

void App::handle_navigation(AppEvent event) {}

void App::handle_file_operations(AppEvent event) {}

void App::handle_dialog_operations(AppEvent event) {}

void App::handle_other_operations(AppEvent event) {};

stdexec::sender auto App::update_directory_preview_async(const int &selected) {
  const auto [width, height] = ftxui::Terminal::Size();
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           ui_.post_task([this] {
             ui_.update_entries_preview(ftxui::text("Loading..."));
           });
         }) |
         stdexec::then([selected, height]() {
           return std::make_pair(selected, height);
         }) |
         stdexec::then([this](const auto pair) {
           return file_manager_.directory_preview(pair);
         }) |
         stdexec::then([this](const auto &entries) {
           return entries_to_elements(entries);
         }) |
         stdexec::then([this](std::vector<ftxui::Element> preview) {
           ui_.post_task([this, pv = std::move(preview)]() {
             ui_.update_entries_preview(ftxui::vbox(pv));
           });
         });
}

stdexec::sender auto App::update_text_preview_async(const int &selected) {
  auto [width, height] = ftxui::Terminal::Size();
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           ui_.post_task([this]() { ui_.update_text_preview("Loading..."); });
         }) |
         stdexec::then([this, selected, width, height]() {
           return file_manager_.text_preview(selected,
                                             std::make_pair(width, height));
         }) |
         stdexec::then([this](std::string preview) {
           ui_.post_task([this, prev = std::move(preview)]() {
             ui_.update_text_preview(prev);
           });
         });
}

void App::refresh_menu_async() {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([this]() { return file_manager_.curdir_entries(); }) |
      stdexec::then([this](const auto &entries) {
        return entries_to_elements(entries);
      }) |
      stdexec::then([this](std::vector<ftxui::Element> elements) {
        ui_.post_task([this, elmt = std::move(elements)]() {
          ui_.update_curdir_entries(elmt);
        });
      });
  stdexec::sync_wait(task);
}

void App::reload_menu_async() {
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([this]() {
                return file_manager_.update_curdir_entries(false);
              }) |
              stdexec::then([this](const auto &entries) {
                return entries_to_elements(entries);
              }) |
              stdexec::then([this](std::vector<ftxui::Element> elements) {
                ui_.post_task([this, elmt = std::move(elements)]() {
                  ui_.update_curdir_entries(elmt);
                  // Remove duplicate event posting - UI update is handled
                  // above
                });
              });
  stdexec::sync_wait(task);
}

void App::update_preview_async() {
  const int selected = ui_.selected();
  const auto selected_path = file_manager_.selected_entry(selected);
  if (selected_path) {
    if (fs::is_directory(selected_path.value())) {
      stdexec::sync_wait(update_directory_preview_async(selected));
    } else {
      stdexec::sync_wait(update_text_preview_async(selected));
    }
  }
}

void App::enter_directory() {
  auto entry = file_manager_.selected_entry(ui_.selected())
                   .and_then([](fs::directory_entry entry)
                                 -> std::optional<fs::directory_entry> {
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
          file_manager_.update_current_path(path);
        }) |
        stdexec::then(
            [this]() { return file_manager_.update_curdir_entries(true); }) |
        stdexec::then([this](const auto &entries) {
          return entries_to_elements(entries);
        }) |
        stdexec::then([this](std::vector<ftxui::Element> elements) {
          ui_.post_task([this, elem = std::move(elements)]() {
            file_data.data = AppEventData::FileOperationData{ui_.selected()};
            ui_.update_and_post(AppEvent::EnterDirectory, file_data);
            update_preview_async();
          });
        });
    stdexec::sync_wait(task);
  }
}

void App::leave_directory() {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([this]() { return file_manager_.cur_parent_path(); }) |
      stdexec::then([this](const fs::path &path) {
        file_manager_.update_current_path(path);
      }) |
      stdexec::then(
          [this]() { return file_manager_.update_curdir_entries(true); }) |
      stdexec::then([this](const auto &entries) {
        return entries_to_elements(entries);
      }) |
      stdexec::then([this](std::vector<ftxui::Element> entries) {
        return std::make_pair(std::move(entries),
                              file_manager_.previous_path_index());
      }) |
      stdexec::then([this](std::pair<std::vector<ftxui::Element>, int> pair) {
        ui_.post_task([this, pair = std::move(pair)]() {
          AppEventData file_data;
          file_data.data = AppEventData::FileOperationData{pair.second};
          ui_.update_and_post(AppEvent::LeaveDirectory, file_data);
        });
        return pair.second;
      }) |
      stdexec::let_value([this](const int &selected) {
        return update_directory_preview_async(selected);
      });
  stdexec::sync_wait(task);
}

void App::confirm_deletion() {
  int selected = ui_.selected();
  if (not file_manager_.marked_entries().empty()) {
    auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                stdexec::then([this]() {
                  return file_manager_.delete_marked_entries();
                }) |
                stdexec::then([this](bool success) {
                  return file_manager_.update_curdir_entries(!success);
                }) |
                stdexec::then([this](const auto &entries) {
                  return entries_to_elements(entries);
                }) |
                stdexec::then([this](std::vector<ftxui::Element> element) {
                  ui_.post_task([this, elem = std::move(element)]() {
                    ui_.update_curdir_entries(elem);
                    // UI update is sufficient for marked entries deletion
                  });
                });
    stdexec::sync_wait(task);
  } else {
    auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                stdexec::then([this, selected]() {
                  return file_manager_.delete_selected_entry(selected);
                }) |
                stdexec::then([this](bool success) {
                  return file_manager_.update_curdir_entries(!success);
                }) |
                stdexec::then([this](const auto &entries) {
                  return entries_to_elements(entries);
                }) |
                stdexec::then([this](std::vector<ftxui::Element> element) {
                  ui_.post_task([this, elem = std::move(element)]() {
                    ui_.update_curdir_entries(elem);
                    // UI update is sufficient for single entry deletion
                  });
                });
    stdexec::sync_wait(task);
  }
}

void App::confirm_creation() {
  int selected = ui_.selected();
  auto filename = ui_.new_entry_input();
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([this, selected, filename]() {
                file_manager_.create_new_entry(filename);
              }) |
              stdexec::then([this]() {
                return file_manager_.update_curdir_entries(false);
              }) |
              stdexec::then([this](const auto &entries) {
                return entries_to_elements(entries);
              }) |
              stdexec::then([this](std::vector<ftxui::Element> element) {
                ui_.post_task([this, elem = std::move(element)]() {
                  ui_.update_curdir_entries(elem);
                  // UI update is sufficient for creation
                });
              });
  stdexec::sync_wait(task);
}

void App::confirm_rename() {
  int selected = ui_.selected();
  auto str = ui_.rename_input();
  auto rename_task = stdexec::schedule(Scheduler::io_scheduler()) |
                     stdexec::then([this, selected, str]() {
                       file_manager_.rename_selected_entry(selected, str);
                     }) |
                     stdexec::then([this]() {
                       return file_manager_.update_curdir_entries(false);
                     }) |
                     stdexec::then([this](const auto &entries) {
                       return entries_to_elements(entries);
                     }) |
                     stdexec::then([this](std::vector<ftxui::Element> element) {
                       ui_.post_task([this, elem = std::move(element)]() {
                         ui_.update_curdir_entries(elem);
                         // UI update is sufficient for rename
                       });
                     });
  stdexec::sync_wait(rename_task);
}

void App::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"},       {".cpp", "nvim"},  {".c", "nvim"},
      {".md", "zen-browser"}, {".json", "nvim"}, {".gitignore", "nvim "}};

  try {
    auto selected_file_opt = file_manager_.selected_entry(ui_.selected());
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
        // 子进程
        const char *handler = handlers.at(ext).c_str();
        const char *file_path = selected_file_opt->path().c_str();

        execlp(handler, handler, file_path, static_cast<char *>(nullptr));

        // 如果execlp失败，记录错误并退出子进程
        std::println(stderr, "[ERROR]: execlp failed for {}: {}", handler,
                     strerror(errno));
        _exit(EXIT_FAILURE);
      } else {
        // 父进程
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
           if (file_manager_.is_marked(entry)) {
             marker = ftxui::text("█ ");
             if (file_manager_.yanking()) {
               marker = ftxui::text("█ ") | ftxui::color(ftxui::Color::Blue);
             } else if (file_manager_.cutting()) {
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
