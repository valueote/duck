#include "duck.hpp"
#include "content_provider.hpp"
#include "file_manager.hpp"
#include "input_handler.hpp"
#include "ui.hpp"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace duck {
Duck::Duck() : input_handler_(ui_), content_provider_(ui_) { setup_ui(); }

void Duck::run() { ui_.render(); }

void Duck::setup_ui() {
  ui_.set_deletion_dialog(content_provider_.deletion_dialog(),
                          input_handler_.deletion_dialog_handler());
  ui_.set_rename_dialog(content_provider_.rename_dialog(),
                        input_handler_.rename_dialog_handler());
  ui_.set_creation_dialog(content_provider_.creation_dialog(),
                          input_handler_.creation_dialog_handler());
  ui_.set_main_layout(content_provider_.layout(),
                      input_handler_.navigation_handler(),
                      input_handler_.operation_handler());
  ui_.set_notification(content_provider_.notification());
  ui_.finalize_tui();

  ui_.update_curdir_entries(ContentProvider::entries_to_elements(
      FileManager::update_curdir_entries(false)));
  input_handler_.update_preview_async();
}
} // namespace duck
