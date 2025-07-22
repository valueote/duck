#include "duck.h"
#include "inputhandler.h"
#include "ui.h"
#include <ftxui/component/event.hpp>

namespace duck {
Duck::Duck()
    : input_handler_(file_manager_, ui_), content_provider(file_manager_, ui_) {
  setup_ui();
}

void Duck::run() { ui_.render(); }

void Duck::setup_ui() {
  ui_.update_curdir_string_entires(file_manager_.curdir_entries_string());
  ui_.set_input_handler(input_handler_.navigation_handler());
  ui_.set_layout(content_provider.preview());
  ui_.set_deletion_dialog(content_provider.deleted_entries(),
                          input_handler_.deletetion_dialog_handler());
}
} // namespace duck
