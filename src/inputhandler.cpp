#include "inputhandler.h"
#include "filemanager.h"
#include <ftxui/component/event.hpp>
#include <print>
#include <sys/wait.h>
namespace duck {
InputHander::InputHander(FileManager &file_manager, UI &ui)
    : file_manager_{file_manager}, ui_{ui} {}
bool InputHander::operator()(ftxui::Event event) {
  if (event == ftxui::Event::Return) {
    open_file();
    return true;
  }
  if (event == ftxui::Event::Character('l')) {
    if (file_manager_.get_selected_entry(ui_.get_selected()).has_value() &&
        fs::is_directory(
            file_manager_.get_selected_entry(ui_.get_selected()).value())) {

      file_manager_.update_current_path(fs::canonical(
          file_manager_.get_selected_entry(ui_.get_selected()).value().path()));
      ui_.move_down_direcotry(file_manager_);
    }
    return true;
  }
  if (event == ftxui::Event::Character('h')) {
    file_manager_.update_current_path(file_manager_.cur_parent_path());
    ui_.move_up_direcotry(file_manager_);
    ui_.set_selected_previous_dir(file_manager_);
    return true;
  }
  if (event == ftxui::Event::Character('q')) {
    ui_.exit();
    return true;
  }
  if (event == ftxui::Event::Character("d")) {
    return true;
  }
  return false;
}

void InputHander::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"},       {".cpp", "nvim"},  {".c", "nvim"},
      {".md", "zen-browser"}, {".json", "nvim"}, {".gitignore", "nvim "}};
  auto selected_file_opt = file_manager_.get_selected_entry(ui_.get_selected());
  if (!selected_file_opt.has_value()) {
    return;
  }

  std::string ext = selected_file_opt.value().path().extension().string();
  if (!handlers.contains(ext)) {
    return;
  }

  ui_.get_screen().WithRestoredIO([&] {
    pid_t pid = fork();
    if (pid == -1) {
      std::print(stderr, "[ERROR]: fork fail in open_file");
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
  })();
}

} // namespace duck
