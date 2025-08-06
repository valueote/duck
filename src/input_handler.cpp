#include "input_handler.h"
#include "duck_event.h"
#include "file_manager.h"
#include "scheduler.h"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <print>
#include <string>
#include <sys/wait.h>
#include <vector>

namespace duck {
InputHandler::InputHandler(Ui &ui) : ui_{ui} {}

// All the input handler is binded to the ui thread, so it's safe to modify ui
// in the navigation_handler

std::function<bool(ftxui::Event)> InputHandler::navigation_handler() {
  return [this](ftxui::Event event) {
    if (event == ftxui::Event::Return) {
      open_file();
      return true;
    }

    if (event == ftxui::Event::Character(' ')) {
      FileManager::toggle_mark_on_selected(ui_.selected());
      ui_.update_curdir_entries_string(FileManager::curdir_entries_string());
      ui_.move_selected_down(FileManager::curdir_entries().size() - 1);
      return true;
    }

    if (event == ftxui::Event::Character('j') ||
        event == ftxui::Event::ArrowDown) {
      ui_.move_selected_down(FileManager::curdir_entries().size() - 1);
      update_preview_async();

      return true;
    }

    if (event == ftxui::Event::Character('k') ||
        event == ftxui::Event::ArrowUp) {
      ui_.move_selected_up(FileManager::curdir_entries().size() - 1);
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

    if (event == ftxui::Event::Character('d')) {
      ui_.toggle_deletion_dialog();
      return true;
    }

    if (event == ftxui::Event::Character('y')) {
      FileManager::start_yanking();
      return true;
    }

    if (event == ftxui::Event::Character('x')) {
      FileManager::start_cutting();
      return true;
    }

    if (event == ftxui::Event::Character('p')) {
      FileManager::paste(ui_.selected());
      scope_.spawn_future(FileManager::update_curdir_entries_async());
      ui_.update_curdir_entries_string(FileManager::curdir_entries_string());
      return true;
    }

    if (event == ftxui::Event::Character('.')) {
      FileManager::toggle_hidden_entries();
      scope_.spawn_future(FileManager::update_curdir_entries_async());
      ui_.update_curdir_entries_string(FileManager::curdir_entries_string());
      return true;
    }

    if (event == ftxui::Event::Escape) {
      FileManager::clear_marked_entries();
      ui_.update_curdir_entries_string(FileManager::curdir_entries_string());
      return true;
    }

    return false;
  };
}

std::function<bool(ftxui::Event)> InputHandler::test_handler() {
  return [this](ftxui::Event event) { return false; };
}

std::function<bool(ftxui::Event)> InputHandler::deletetion_dialog_handler() {
  return [this](ftxui::Event event) {
    if (event == ftxui::Event::Character('y')) {
      if (!FileManager::marked_entries().empty()) {
        FileManager::delete_marked_entries();
      } else {
        FileManager::delete_selected_entry(ui_.selected());
      }
      scope_.spawn_future(FileManager::update_curdir_entries_async());
      ui_.update_curdir_entries_string(FileManager::curdir_entries_string());
      ui_.toggle_deletion_dialog();
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

void InputHandler::enter_direcotry() {
  auto entry =
      FileManager::selected_entry(ui_.selected())
          .and_then([](fs::directory_entry entry)
                        -> std::expected<fs::directory_entry, std::string> {
            if (fs::is_directory(entry)) {
              return std::expected<fs::directory_entry, std::string>{
                  std::move(entry)};
            } else {
              return std::unexpected<std::string>{""};
            }
          });

  if (entry) {
    auto task = stdexec::on(
        Scheduler::io_scheduler(),
        FileManager::update_current_path_async(entry.value().path()) |
            stdexec::then(FileManager::format_entries) |
            stdexec::then([this](std::vector<std::string> entries) {
              ui_.post_task([this, entries]() {
                ui_.enter_direcotry(std::move(entries));
                ui_.post_event(DuckEvent::refresh);
                update_preview_async();
              });
            }));
    scope_.spawn(std::move(task));
  }
}

void InputHandler::leave_direcotry() {
  auto task = stdexec::on(
      Scheduler::io_scheduler(),
      FileManager::update_current_path_async(FileManager::cur_parent_path()) |
          stdexec::then([](const std::vector<fs::directory_entry> &entries) {
            return FileManager::format_entries(entries);
          }) |
          stdexec::then([this](std::vector<std::string> entries) {
            return std::make_pair(std::move(entries),
                                  FileManager::previous_path_index());
          }) |
          stdexec::then(
              [this](
                  std::pair<std::vector<std::string>, int> entries_and_index) {
                ui_.post_task([this, entries_and_index]() {
                  ui_.leave_direcotry(entries_and_index.first,
                                      entries_and_index.second);
                  ui_.post_event(DuckEvent::refresh);
                });
                return entries_and_index.second;
              }) |
          stdexec::then(FileManager::directory_preview) |
          stdexec::then([this](ftxui::Element preview) {
            ui_.post_task([this, preview]() {
              ui_.update_entries_preview(std::move(preview));
            });
          }));
  scope_.spawn(std::move(task));
}

stdexec::sender auto
InputHandler::update_directory_preview_async(const int &selected) {
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           ui_.post_task([this]() {
             ui_.update_entries_preview(ftxui::text("Loading..."));
           });
         }) |
         stdexec::then([this, selected]() {
           return FileManager::directory_preview(selected);
         }) |
         stdexec::then([this](ftxui::Element preview) {
           ui_.post_task([this, preview]() {
             ui_.update_entries_preview(std::move(preview));
           });
         });
}

stdexec::sender auto
InputHandler::update_text_preview_async(const int &selected) {
  return stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([this]() {
           ui_.post_task([this]() { ui_.update_text_preview("Loading..."); });
         }) |
         stdexec::then([this, selected]() {
           return FileManager::text_preview(selected);
         }) |
         stdexec::then([this](std::string preview) {
           ui_.post_task([this, preview]() {
             ui_.update_text_preview(std::move(preview));
           });
         });
}

void InputHandler::update_preview_async() {
  const int selected = ui_.selected();
  const auto selected_path = FileManager::selected_entry(selected);
  if (selected_path) {
    if (fs::is_directory(selected_path.value())) {
      auto task = update_directory_preview_async(selected);
      scope_.spawn(std::move(task));
    } else {
      auto task = update_text_preview_async(selected);
      scope_.spawn(std::move(task));
    }
  }
}

void InputHandler::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"},       {".cpp", "nvim"},  {".c", "nvim"},
      {".md", "zen-browser"}, {".json", "nvim"}, {".gitignore", "nvim "}};
  auto selected_file_opt = FileManager::selected_entry(ui_.selected());
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
      int status;
      waitpid(pid, &status, 0);
    }
  });
}

} // namespace duck
