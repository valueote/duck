#pragma once
#include "app_event.hpp"
#include "app_state.hpp"
#include "content_provider.hpp"
#include "input_handler.hpp"
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <functional>
#include <stack>
#include <string>

namespace duck {

class Ui {
private:
  MenuInfo info_;
  EntryPreview preview_;
  ContentProvider content_provider_;
  ftxui::Element selected_entries_;
  InputHandler &input_handler_;

  ftxui::ScreenInteractive screen_;
  ftxui::Component tui_;
  ftxui::Component main_layout_;
  ftxui::Component deletion_dialog_;
  ftxui::Component creation_dialog_;
  ftxui::Component notification_;
  std::string notification_content_;
  ftxui::Component rename_dialog_;
  std::string input_content_;
  int cursor_positon_;

  std::stack<int> index_selected_;

  enum class pane : uint8_t {
    MAIN = 0,
    DELETION,
    RENAME,
    CREATION,
    NOTIFICATION
  };
  int active_pane_;

  void finalize_tui();

public:
  Ui(InputHandler &input_handler);
  void update_whole_state(const AppState &state);

  void toggle_deletion_dialog();
  void toggle_rename_dialog();
  void toggle_creation_dialog();
  void toggle_notification();

  void async_update_info(MenuInfo new_info);
  void async_update_index(size_t index);
  void async_update_selected(ftxui::Element selected_entries);
  void async_update_preview(EntryPreview new_preview);
  void update_rename_input(std::string input);
  void update_notification(std::string input);

  std::string &input_content();
  int &cursor_positon();

  void render(AppState &state);
  void exit();
  void post_task(std::function<void()> task);
  void restored_io(std::function<void()> closure);
};

} // namespace duck
