module;
#include <ftxui/component/event.hpp>
module duck.core;

import duck.inputhandler;
import duck.ui;
import duck.contentprovider;

namespace duck {
Duck::Duck()
    : input_handler_(file_manager_, ui_),
      content_provider_(file_manager_, ui_, color_scheme_) {
  setup_ui();
}

void Duck::run() { ui_.render(); }

void Duck::setup_ui() {
  ui_.set_menu(content_provider_.entries_transform());
  ui_.update_curdir_entries_string(file_manager_.curdir_entries_string());
  ui_.set_input_handler(input_handler_.navigation_handler());
  ui_.set_layout(content_provider_.preview());
  ui_.set_deletion_dialog(content_provider_.deletion_dialog(),
                          input_handler_.deletetion_dialog_handler());
}
} // namespace duck
