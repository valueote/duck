#include "duck.h"
#include "file_manager.h"
#include "input_handler.h"
#include "ui.h"
#include <ftxui/component/event.hpp>

namespace duck {
Duck::Duck() : input_handler_(ui_), content_provider_(ui_, color_scheme_) {
  setup_ui();
}

void Duck::run() { ui_.render(); }

void Duck::setup_ui() {
  ui_.set_menu(content_provider_.entries_transform());
  ui_.update_curdir_entries_string(
      FileManager::format_entries(FileManager::update_curdir_entries()));
  ui_.set_input_handler(input_handler_.navigation_handler(),
                        input_handler_.test_handler());
  ui_.set_layout(content_provider_.preview());
  ui_.set_deletion_dialog(content_provider_.deletion_dialog(),
                          input_handler_.deletetion_dialog_handler());
  input_handler_.update_preview_async();
}
} // namespace duck
