#include "input_handler.h"
#include "duck_event.h"
#include "file_manager.h"
#include "scheduler.h"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>
#include <functional>
#include <print>
#include <string>
#include <sys/wait.h>
#include <utility>
#include <vector>

// All the input handler is binded to the ui thread, so it's safe to modify and
// access data of ui in the handler without lock

namespace duck {
using std::exit;
using std::move;

InputHandler::InputHandler(Ui &ui) : ui_{ui} {}

std::function<bool(const ftxui::Event &)> InputHandler::navigation_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Character('j') ||
        event == ftxui::Event::ArrowDown) {
      ui_.move_selected_down(
          static_cast<int>(FileManager::curdir_entries().size()));
      update_preview_async();

      return true;
    }

    if (event == ftxui::Event::Character('k') ||
        event == ftxui::Event::ArrowUp) {
      ui_.move_selected_up(
          static_cast<int>(FileManager::curdir_entries().size()));
      update_preview_async();
      return true;
    }

    if (event == ftxui::Event::Character('l')) {
      enter_direcotry();
      return true;
    }

    if (event == ftxui::Event::Character('h')) {
      leave_direcotry();
      return true;
    }

    if (event == ftxui::Event::Character('q')) {
      ui_.exit();
      return true;
    }
    if (event == ftxui::Event::Escape) {
      auto selected = ui_.global_selected();
      auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                  stdexec::then(FileManager::clear_marked_entries) |
                  stdexec::then([this]() { refresh_menu_async(); });

      scope_.spawn(task);
      return true;
    }

    return false;
  };
}

std::function<bool(ftxui::Event)> InputHandler::operation_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Return) {
      open_file();
      return true;
    }

    if (event == ftxui::Event::Character(' ')) {
      auto global_selected = ui_.global_selected();
      auto task =
          stdexec::schedule(Scheduler::io_scheduler()) |
          stdexec::then([this, global_selected]() { return global_selected; }) |
          stdexec::then(FileManager::toggle_mark_on_selected) |
          stdexec::then(FileManager::curdir_entries) |
          stdexec::then(FileManager::entries_to_elements) |
          stdexec::then([this](std::vector<ftxui::Element> elements) {
            ui_.post_task([this, elmt = std::move(elements)]() {
              ui_.update_curdir_entries(elmt);
              ui_.move_selected_down(
                  static_cast<int>(FileManager::curdir_entries().size()));
              ui_.post_event(DuckEvent::refresh);
              update_preview_async();
            });
          });

      scope_.spawn(task);
      return true;
    }

    if (event == ftxui::Event::Character('d')) {
      ui_.toggle_deletion_dialog();
      return true;
    }

    if (event == ftxui::Event::Character('a')) {
      ui_.toggle_creation_dialog();
      return true;
    }

    if (event == ftxui::Event::Character('y')) {
      auto global_selected = ui_.global_selected();
      auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                  stdexec::then([global_selected]() {
                    FileManager::start_yanking(global_selected);
                  }) |
                  stdexec::then([this]() { refresh_menu_async(); });
      scope_.spawn(task);
      return true;
    }

    if (event == ftxui::Event::Character('x')) {
      auto global_selected = ui_.global_selected();
      auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                  stdexec::then([global_selected]() {
                    FileManager::start_cutting(global_selected);
                  }) |
                  stdexec::then([this]() { refresh_menu_async(); });
      scope_.spawn(task);
      return true;
    }

    if (event == ftxui::Event::Character('r')) {
      ui_.update_rename_input(FileManager::selected_entry(ui_.global_selected())
                                  .value()
                                  .path()
                                  .filename()
                                  .string());

      ui_.toggle_rename_dialog();
      return true;
    }

    if (event == ftxui::Event::Character('p')) {
      auto selected = ui_.global_selected();
      auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                  stdexec::then([selected]() { return selected; }) |
                  stdexec::then(FileManager::yank_or_cut) |
                  stdexec::then([this]() { refresh_menu_async(); });
      scope_.spawn_future(task);
      return true;
    }

    if (event == ftxui::Event::Character('.')) {
      auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                  stdexec::then(FileManager::toggle_hidden_entries) |
                  stdexec::then([this]() { refresh_menu_async(); });
      scope_.spawn_future(task);
      return true;
    }

    return false;
  };
}

std::function<bool(const ftxui::Event &)>
InputHandler::deletion_dialog_handler() {
  return [this](const ftxui::Event &event) {
    auto selected = ui_.global_selected();
    if (event == ftxui::Event::Character('y')) {
      if (not FileManager::marked_entries().empty()) {
        auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                    stdexec::then(FileManager::delete_marked_entries) |
                    stdexec::then([](bool success) {
                      return FileManager::update_curdir_entries(false);
                    }) |
                    stdexec::then(FileManager::entries_to_elements) |
                    stdexec::then([this](std::vector<ftxui::Element> element) {
                      ui_.post_task([this, elem = std::move(element)]() {
                        ui_.update_curdir_entries(elem);
                        ui_.post_event(DuckEvent::refresh);
                        ui_.toggle_deletion_dialog();
                      });
                    });

        scope_.spawn(task);
      } else {
        auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                    stdexec::then([this]() { return ui_.global_selected(); }) |
                    stdexec::then(FileManager::delete_selected_entry) |
                    stdexec::then([](bool success) {
                      return FileManager::update_curdir_entries(false);
                    }) |
                    stdexec::then(FileManager::entries_to_elements) |
                    stdexec::then([this](std::vector<ftxui::Element> element) {
                      ui_.post_task([this, elem = std::move(element)]() {
                        ui_.update_curdir_entries(elem);
                        ui_.post_event(DuckEvent::refresh);
                        ui_.toggle_deletion_dialog();
                      });
                    });

        scope_.spawn(task);
      }
      return true;
    }

    if (event == ftxui::Event::Character('n') ||
        event == ftxui::Event::Escape) {
      ui_.toggle_deletion_dialog();
      return true;
    }

    return false;
  };
}

std::function<bool(const ftxui::Event &)>
InputHandler::rename_dialog_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Escape) {
      ui_.toggle_rename_dialog();
      return true;
    }

    if (event == ftxui::Event::Return) {
      int selected = ui_.global_selected();
      auto str = ui_.rename_input();
      auto rename_task =
          stdexec::schedule(Scheduler::io_scheduler()) |
          stdexec::then([selected, str]() {
            FileManager::rename_selected_entry(selected, str);
          }) |
          stdexec::then(
              []() { return FileManager::update_curdir_entries(false); }) |
          stdexec::then(FileManager::entries_to_elements) |
          stdexec::then([this](std::vector<ftxui::Element> element) {
            ui_.post_task([this, elem = std::move(element)]() {
              ui_.update_curdir_entries(elem);
              ui_.post_event(DuckEvent::refresh);
              ui_.toggle_rename_dialog();
            });
          });

      scope_.spawn(rename_task);
      return true;
    }
    return false;
  };
}

std::function<bool(const ftxui::Event &)>
InputHandler::creation_dialog_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Escape) {
      ui_.toggle_creation_dialog();
      return true;
    }

    if (event == ftxui::Event::Return) {
      return true;
    }
    return false;
  };
}

stdexec::sender auto
InputHandler::update_directory_preview_async(const int &selected) {
  const auto [width, height] = ftxui::Terminal::Size();
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           ui_.post_task([this] {
             ui_.update_entries_preview(ftxui::text("Loading..."));
           });
         }) |
         stdexec::then([selected, height]() {
           return std::make_pair(selected, height);
         }) |
         stdexec::then(FileManager::directory_preview) |
         stdexec::then(FileManager::entries_to_element) |
         stdexec::then([this](ftxui::Element preview) {
           ui_.post_task([this, pv = std::move(preview)]() {
             ui_.update_entries_preview(pv);
             ui_.post_event(DuckEvent::refresh);
           });
         });
}

stdexec::sender auto
InputHandler::update_text_preview_async(const int &selected) {
  auto [width, height] = ftxui::Terminal::Size();
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           ui_.post_task([this]() { ui_.update_text_preview("Loading..."); });
         }) |
         stdexec::then([selected, width, height]() {
           return FileManager::text_preview(selected,
                                            std::make_pair(width, height));
         }) |
         stdexec::then([this](std::string preview) {
           ui_.post_task([this, prev = std::move(preview)]() {
             ui_.update_text_preview(prev);
             ui_.post_event(DuckEvent::refresh);
           });
         });
}

// Just read the curdir_entries and update the menu entries
void InputHandler::refresh_menu_async() {
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then(FileManager::curdir_entries) |
              stdexec::then(FileManager::entries_to_elements) |
              stdexec::then([this](std::vector<ftxui::Element> elements) {
                ui_.post_task([this, elmt = std::move(elements)]() {
                  ui_.update_curdir_entries(elmt);
                  ui_.post_event(ftxui::Event::Custom);
                });
              });
  scope_.spawn(task);
}

void InputHandler::update_preview_async() {
  const int selected = ui_.global_selected();
  const auto selected_path = FileManager::selected_entry(selected);
  if (selected_path) {
    if (fs::is_directory(selected_path.value())) {
      scope_.spawn(update_directory_preview_async(selected));
    } else {
      scope_.spawn(update_text_preview_async(selected));
    }
  }
}

void InputHandler::enter_direcotry() {
  auto entry =
      FileManager::selected_entry(ui_.global_selected())
          .and_then([](fs::directory_entry entry)
                        -> std::expected<fs::directory_entry, std::string> {
            if (fs::is_directory(entry)) {
              return std::expected<fs::directory_entry, std::string>{
                  std::move(entry)};
            }
            return std::unexpected<std::string>{""};
          });

  if (entry) {
    auto task =
        stdexec::schedule(Scheduler::io_scheduler()) |
        stdexec::then([et = std::move(entry)]() { return et.value().path(); }) |
        stdexec::then(FileManager::update_current_path) | stdexec::then([]() {
          return FileManager::update_curdir_entries(true);
        }) |
        stdexec::then(FileManager::entries_to_elements) |
        stdexec::then([this](std::vector<ftxui::Element> elements) {
          ui_.post_task([this, elem = std::move(elements)]() {
            ui_.enter_direcotry(elem);
            ui_.post_event(DuckEvent::refresh);
            update_preview_async();
          });
        });

    scope_.spawn(std::move(task));
  }
}

void InputHandler::leave_direcotry() {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then(FileManager::cur_parent_path) |
      stdexec::then(FileManager::update_current_path) |
      stdexec::then([]() { return FileManager::update_curdir_entries(true); }) |
      stdexec::then(FileManager::entries_to_elements) |
      stdexec::then([this](std::vector<ftxui::Element> entries) {
        return std::make_pair(std::move(entries),
                              FileManager::previous_path_index());
      }) |
      stdexec::then([this](std::pair<std::vector<ftxui::Element>, int> pair) {
        ui_.post_task([this, pair = std::move(pair)]() {
          ui_.leave_direcotry(pair.first, pair.second);
          ui_.post_event(DuckEvent::refresh);
        });
        return pair.second;
      }) |
      stdexec::let_value([this](const int &selected) {
        return update_directory_preview_async(selected);
      });
  scope_.spawn(task);
}

void InputHandler::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"},       {".cpp", "nvim"},  {".c", "nvim"},
      {".md", "zen-browser"}, {".json", "nvim"}, {".gitignore", "nvim "}};
  auto selected_file_opt = FileManager::selected_entry(ui_.global_selected());
  if (!selected_file_opt.has_value()) {
    return;
  }

  std::string ext = selected_file_opt.value().path().extension().string();
  if (!handlers.contains(ext)) {
    return;
  }

  ui_.restored_io([&]() {
    pid_t pid = fork();
    if (pid == -1) {
      std::println(stderr, "[ERROR]: fork fail in open_file");
      return;
    }

    if (pid == 0) {
      const char *handler = handlers.at(ext).c_str();
      const char *file_path = selected_file_opt->path().c_str();

      execlp(handler, handler, file_path, nullptr);

      std::exit(EXIT_FAILURE);
    } else {
      int status{0};
      waitpid(pid, &status, 0);
    }
  });
}

} // namespace duck
