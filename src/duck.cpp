#include "duck.h"
#include <ftxui/component/event.hpp>

namespace duck {
Duck::Duck() {
  ui_.update_curdir_string_entires(file_manager_);
  ui_.set_input_handler([this](ftxui::Event event) {
    if (event == ftxui::Event::Return) {
      ui_.open_file(file_manager_);
      return true;
    }

    if (event == ftxui::Event::Character('l')) {
      if (file_manager_.get_selected_entry(ui_.get_selected()).has_value() &&
          fs::is_directory(
              file_manager_.get_selected_entry(ui_.get_selected()).value())) {
        ui_.move_down_direcotry(file_manager_);
      }
      return true;
    }

    if (event == ftxui::Event::Character('h')) {
      ui_.move_up_direcotry(file_manager_);
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
  });

  ui_.setup_layout(file_manager_);
}

void Duck::run() { ui_.render(); }

} // namespace duck
