#include "inputhandler.h"
#include "filemanager.h"
#include <ftxui/component/event.hpp>
namespace duck {
InputHander::InputHander(FileManager &file_manager, UI &ui)
    : file_manager_{file_manager}, ui_{ui} {}
bool InputHander::operator()(ftxui::Event event) {
  if (event == ftxui::Event::Return) {
    ui_.open_file(file_manager_);
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

} // namespace duck
