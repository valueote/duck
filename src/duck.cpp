#include "duck.h"
#include "inputhandler.h"
#include "layoutbuilder.h"
#include "ui.h"
#include <ftxui/component/event.hpp>

namespace duck {
Duck::Duck()
    : input_handler_(file_manager_, ui_), ui_builder_(file_manager_, ui_) {
  setup_ui();
}

void Duck::run() { ui_.render(); }

void Duck::setup_ui() {
  ui_.update_curdir_string_entires(file_manager_.curdir_entries());
  ui_.set_input_handler(input_handler_.navigation_handler());
  ui_.set_layout(ui_builder_.layout());
  ui_.set_deletion_dialog(ui_builder_.deletion_dialog(),
                          input_handler_.deletetion_handler());
}
} // namespace duck
