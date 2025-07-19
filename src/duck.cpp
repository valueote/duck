#include "duck.h"
#include "inputhandler.h"
#include "ui.h"
#include <ftxui/component/event.hpp>

namespace duck {
Duck::Duck() : input_handler(file_manager_, ui_) { setup_ui(); }

void Duck::run() { ui_.render(); }

void Duck::setup_ui() {
  ui_.update_curdir_string_entires(file_manager_);
  ui_.set_input_handler(input_handler);
  ui_.setup_layout(file_manager_);
}
} // namespace duck
