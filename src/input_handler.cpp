#include "input_handler.hpp"
#include "colorscheme.hpp"
#include "file_manager.hpp"
#include "scheduler.hpp"
#include "stdexec/stop_token.hpp"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>
#include <functional>
#include <optional>
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

InputHandler::InputHandler(Ui &ui, FileManager &file_manager)
    : ui_{ui}, file_manager_{file_manager} {}

std::function<bool(const ftxui::Event &)> InputHandler::navigation_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Character('j') ||
        event == ftxui::Event::ArrowDown) {
      ui_.move_selected_down(
          static_cast<int>(file_manager_.curdir_entries().size()));
      update_preview_async();

      return true;
    }

    if (event == ftxui::Event::Character('k') ||
        event == ftxui::Event::ArrowUp) {
      ui_.move_selected_up(
          static_cast<int>(file_manager_.curdir_entries().size()));
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
      auto selected = ui_.selected();
      auto task =
          stdexec::schedule(Scheduler::io_scheduler()) |
          stdexec::then([this]() { file_manager_.clear_marked_entries(); }) |
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
      auto global_selected = ui_.selected();
      auto task =
          stdexec::schedule(Scheduler::io_scheduler()) |
          stdexec::then([this, global_selected]() {
            file_manager_.toggle_mark_on_selected(global_selected);
          }) |
          stdexec::then([this]() { return file_manager_.curdir_entries(); }) |
          stdexec::then([this](const auto &entries) {
            return entries_to_elements(entries);
          }) |
          stdexec::then([this](std::vector<ftxui::Element> elements) {
            ui_.post_task([this, elmt = std::move(elements)]() {
              ui_.update_curdir_entries(elmt);
              ui_.move_selected_down(
                  static_cast<int>(file_manager_.curdir_entries().size()));
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
      auto selected = ui_.selected();
      auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                  stdexec::then([this, selected]() {
                    file_manager_.start_yanking(selected);
                  }) |
                  stdexec::then([this]() { refresh_menu_async(); });
      scope_.spawn(task);
      return true;
    }

    if (event == ftxui::Event::Character('x')) {
      auto selected = ui_.selected();
      auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                  stdexec::then([this, selected]() {
                    file_manager_.start_cutting(selected);
                  }) |
                  stdexec::then([this]() { refresh_menu_async(); });
      scope_.spawn(task);
      return true;
    }

    if (event == ftxui::Event::Character('r')) {
      ui_.update_rename_input(file_manager_.selected_entry(ui_.selected())
                                  .value()
                                  .path()
                                  .filename()
                                  .string());

      ui_.toggle_rename_dialog();
      return true;
    }

    if (event == ftxui::Event::Character('p')) {
      auto selected = ui_.selected();
      auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                  stdexec::then([selected]() { return selected; }) |
                  stdexec::then([this](const int selected) {
                    file_manager_.yank_or_cut(selected);
                  }) |
                  stdexec::then([this]() { reload_menu_async(); });
      scope_.spawn_future(task);
      return true;
    }

    if (event == ftxui::Event::Character('.')) {
      std::print(stderr, "start  toggle hidden entries");
      auto task =
          stdexec::schedule(Scheduler::io_scheduler()) |
          stdexec::then([this]() { file_manager_.toggle_hidden_entries(); }) |
          stdexec::then([this]() { refresh_menu_async(); });
      scope_.spawn_future(task);
      return true;
    }

    if (event == ftxui::Event::Character('n')) {
      ui_.update_notification("Hello! this is a test for notificaton.");
      ui_.toggle_notification();
      return true;
    }

    return false;
  };
}

std::function<bool(const ftxui::Event &)>
InputHandler::deletion_dialog_handler() {
  return [this](const ftxui::Event &event) {
    auto selected = ui_.selected();
    if (event == ftxui::Event::Character('y')) {
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
                        ui_.toggle_deletion_dialog();
                      });
                    });

        scope_.spawn(task);
      } else {
        auto task = stdexec::schedule(Scheduler::io_scheduler()) |
                    stdexec::then([this]() { return ui_.selected(); }) |
                    stdexec::then([this](const int selected) {
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
      int selected = ui_.selected();
      auto str = ui_.rename_input();
      auto rename_task =
          stdexec::schedule(Scheduler::io_scheduler()) |
          stdexec::then([this, selected, str]() {
            file_manager_.rename_selected_entry(selected, str);
          }) |
          stdexec::then(
              [this]() { return file_manager_.update_curdir_entries(false); }) |
          stdexec::then([this](const auto &entries) {
            return entries_to_elements(entries);
          }) |
          stdexec::then([this](std::vector<ftxui::Element> element) {
            ui_.post_task([this, elem = std::move(element)]() {
              ui_.update_curdir_entries(elem);
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
                      ui_.toggle_creation_dialog();
                    });
                  });

      scope_.spawn(task);

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

stdexec::sender auto
InputHandler::update_text_preview_async(const int &selected) {
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

// Just read the curdir_entries and update the menu entries
void InputHandler::refresh_menu_async() {
  auto token = get_token();
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([this, token]() {
                if (token.stop_requested()) {
                }
                return file_manager_.curdir_entries();
              }) |
              stdexec::then([this](const auto &entries) {
                return entries_to_elements(entries);
              }) |
              stdexec::then([this](std::vector<ftxui::Element> elements) {
                ui_.post_task([this, elmt = std::move(elements)]() {
                  ui_.update_curdir_entries(elmt);
                });
              });
  scope_.spawn(task);
}

void InputHandler::reload_menu_async() {
  auto token = get_token();
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([this, token]() {
                if (token.stop_requested()) {
                }
                return file_manager_.update_curdir_entries(false);
              }) |
              stdexec::then([this](const auto &entries) {
                return entries_to_elements(entries);
              }) |
              stdexec::then([this](std::vector<ftxui::Element> elements) {
                ui_.post_task([this, elmt = std::move(elements)]() {
                  ui_.update_curdir_entries(elmt);
                });
              });
  scope_.spawn(task);
}

void InputHandler::update_preview_async() {
  const int selected = ui_.selected();
  const auto selected_path = file_manager_.selected_entry(selected);
  if (selected_path) {
    if (fs::is_directory(selected_path.value())) {
      scope_.spawn(update_directory_preview_async(selected));
    } else {
      scope_.spawn(update_text_preview_async(selected));
    }
  }
}

void InputHandler::enter_direcotry() {
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
            ui_.enter_direcotry(elem);
            update_preview_async();
          });
        });

    scope_.spawn(std::move(task));
  }
}

void InputHandler::leave_direcotry() {
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
          // FIX:: Passing previous_path_index is dumb, need change
          ui_.leave_direcotry(pair.first, pair.second);
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
  auto selected_file_opt = file_manager_.selected_entry(ui_.selected());
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

std::vector<ftxui::Element> InputHandler::entries_to_elements(
    const std::vector<fs::directory_entry> &entries) {
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
               marker |= ftxui::color(ftxui::Color::Blue);
             } else if (file_manager_.cutting()) {
               marker |= ftxui::color(ftxui::Color::Red);
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

stdexec::inplace_stop_token InputHandler::get_token() {
  if (!stop_source_ || stop_source_->stop_requested()) {
    stop_source_.emplace();
  }
  return stop_source_->get_token();
}

} // namespace duck
