#pragma once
#include "stdexec/__detail/__run_loop.hpp"
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <functional>
#include <shared_mutex>
#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace duck {

class Ui {
private:
  std::vector<std::string> curdir_string_entries_;
  std::vector<std::string> entires_view_;

  std::shared_mutex ui_lock_;
  std::string text_preview_;
  ftxui::Element entries_preview_;

  ftxui::ScreenInteractive screen_;
  ftxui::Component main_layout_;
  ftxui::MenuOption menu_option_;
  ftxui::Component tui_;
  ftxui::Component menu_;

  ftxui::Component deletion_dialog_;
  ftxui::Component rename_dialog_;
  std::string rename_input_;

  std::stack<int> previous_selected_;
  int global_selected_;
  int view_selected_;

  bool show_deletion_dialog_;
  int rename_cursor_positon_;
  bool show_rename_dialog_;
  enum class pane : int { MAIN = 0, DELETION, RENAME };
  int active_pane_;

  stdexec::run_loop loop_;

public:
  Ui();
  void set_menu(std::function<ftxui::Element(const ftxui::EntryState &state)>);
  void set_layout(const std::function<ftxui::Element()> preview);
  void
  set_input_handler(const std::function<bool(const ftxui::Event &)> handler,
                    const std::function<bool(const ftxui::Event &)> test);
  void
  set_deletion_dialog(const ftxui::Component deletion_dialog,
                      const std::function<bool(const ftxui::Event &)> handler);

  void
  set_rename_dialog(const ftxui::Component rename_dialog,
                    const std::function<bool(const ftxui::Event &)> handler);
  void finalize_layout();

  void move_selected_up(const int max);
  void move_selected_down(const int max);
  void toggle_deletion_dialog();
  void toggle_rename_dialog();
  void toggle_hidden_entries();
  void enter_direcotry(std::vector<std::string> curdir_entries_string);
  void leave_direcotry(std::vector<std::string> curdir_entries_string,
                       const int &previous_path_index);
  void
  update_curdir_entries_string(std::vector<std::string> curdir_entries_string);
  void update_entries_preview(ftxui::Element new_entries);
  void update_rename_input(std::string str);
  ftxui::Element entries_preview();

  void update_text_preview(std::string new_text_preview);
  std::string text_preview();
  std::string &rename_input();
  int &rename_cursor_positon();

  void render();
  void exit();
  int selected();
  bool show_hidden();
  void post_event(ftxui::Event event);
  void post_task(std::function<void()> task);
  void restored_io(std::function<void()> closure);
  std::pair<int, int> screen_size();
  ftxui::Component &menu();
};

} // namespace duck
