#include "duck.h"
#include "inputhandler.h"
#include "layoutbuilder.h"
#include "ui.h"
#include <ftxui/component/event.hpp>

namespace duck {
Duck::Duck()
    : input_handler_(file_manager_, ui_), layout_builder_(file_manager_, ui_) {
  setup_ui();
}

void Duck::run() { ui_.render(); }

void Duck::setup_ui() {
  ui_.update_curdir_string_entires(file_manager_);
  ui_.set_input_handler(input_handler_);
  ui_.setup_layout(layout_builder_);
}
} // namespace duck
