#include "inputhandler.h"
#include "filemanager.h"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <print>
#include <sys/wait.h>

namespace duck {
InputHandler::InputHandler(FileManager &file_manager, Ui &ui)
    : file_manager_{file_manager}, ui_{ui} {}

std::function<bool(ftxui::Event)> InputHandler::navigation_handler() {
  return [this](ftxui::Event event) {
    if (event == ftxui::Event::Return) {
      open_file();
      return true;
    }

    if (event == ftxui::Event::Character(' ')) {
      file_manager_.toggle_mark_on_selected(ui_.selected());
      ui_.update_curdir_entries_string(file_manager_.curdir_entries_string());
      ui_.move_selected_down(file_manager_.curdir_entries().size() - 1);
      return true;
    }

    if (event == ftxui::Event::Character('j') ||
        event == ftxui::Event::ArrowDown) {
      ui_.move_selected_down(file_manager_.curdir_entries().size() - 1);
      return true;
    }

    if (event == ftxui::Event::Character('k') ||
        event == ftxui::Event::ArrowUp) {
      ui_.move_selected_up(file_manager_.curdir_entries().size() - 1);
      return true;
    }

    if (event == ftxui::Event::Character('l')) {
      if (file_manager_.get_selected_entry(ui_.selected()).has_value() &&
          fs::is_directory(
              file_manager_.get_selected_entry(ui_.selected()).value())) {

        file_manager_.update_current_path(fs::canonical(
            file_manager_.get_selected_entry(ui_.selected()).value().path()));
        ui_.enter_direcotry(file_manager_.curdir_entries_string());
      }
      return true;
    }

    if (event == ftxui::Event::Character('h')) {
      file_manager_.update_current_path(file_manager_.cur_parent_path());
      ui_.leave_direcotry(file_manager_.curdir_entries_string(),
                          file_manager_.get_previous_path_index());
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
      file_manager_.start_yanking();
      return true;
    }

    if (event == ftxui::Event::Character('x')) {
      file_manager_.start_cutting();
      return true;
    }

    if (event == ftxui::Event::Character('p')) {
      file_manager_.paste(ui_.selected());
      file_manager_.update_curdir_entries();
      ui_.update_curdir_entries_string(file_manager_.curdir_entries_string());
      return true;
    }

    if (event == ftxui::Event::Character('.')) {
      file_manager_.toggle_hidden_entries();
      ui_.update_curdir_entries_string(file_manager_.curdir_entries_string());
    }

    if (event == ftxui::Event::Escape) {
      file_manager_.clear_marked_entries();
      ui_.update_curdir_entries_string(file_manager_.curdir_entries_string());
    }

    return false;
  };
}

std::function<bool(ftxui::Event)> InputHandler::deletetion_dialog_handler() {
  return [this](ftxui::Event event) {
    if (event == ftxui::Event::Character('y')) {
      if (!file_manager_.marked_entries().empty()) {
        file_manager_.delete_marked_entries();
      } else {
        file_manager_.delete_selected_entry(ui_.selected());
      }
      file_manager_.update_curdir_entries();
      ui_.update_curdir_entries_string(file_manager_.curdir_entries_string());
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

void InputHandler::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"},       {".cpp", "nvim"},  {".c", "nvim"},
      {".md", "zen-browser"}, {".json", "nvim"}, {".gitignore", "nvim "}};
  auto selected_file_opt = file_manager_.get_selected_entry(ui_.selected());
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
