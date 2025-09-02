#pragma once
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
#include <vector>

namespace duck {

class Ui {
private:
  ContentProvider content_provider_;
  InputHandler &input_handler_;
  std::vector<ftxui::Element> curdir_entries_;

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

  enum class pane : int8_t {
    MAIN = 0,
    DELETION,
    RENAME,
    CREATION,
    NOTIFICATION
  };
  int active_pane_;

public:
  Ui(InputHandler &input_handler);
  void set_main_layout(const AppState &state);
  void set_deletion_dialog(const AppState &state);

  void finalize_tui();

  void toggle_deletion_dialog();
  void toggle_rename_dialog();
  void toggle_creation_dialog();
  void toggle_notification();

  void enter_direcotry(AppState &state,
                       std::vector<ftxui::Element> curdir_entries);
  void leave_direcotry(AppState &state,
                       std::vector<ftxui::Element> curdir_entries,
                       int previous_path_index);
  void update_curdir_entries(std::vector<ftxui::Element> new_entries);
  void update_rename_input(std::string str);
  void update_notification(std::string str);

  std::string &input_content();
  std::string notification_content();
  int &cursor_positon();

  void render(const AppState &state);
  void exit();
  void post_task(std::function<void()> task);
  void restored_io(std::function<void()> closure);
};

} // namespace duck
