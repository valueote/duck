#pragma once
#include "stdexec/__detail/__run_loop.hpp"
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <functional>
#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace duck {

class Ui {
private:
  std::mutex post_mutex_;
  std::vector<std::string> curdir_string_entries_;

  std::string text_preview_content_;
  ftxui::Element entries_preview_;

  ftxui::ScreenInteractive screen_;
  ftxui::Component main_layout_;
  ftxui::MenuOption menu_option_;
  ftxui::Component modal_;
  ftxui::Component menu_;

  std::stack<int> previous_selected_;
  int selected_;
  bool show_deletion_dialog_;

  stdexec::run_loop loop_;

public:
  Ui();
  void set_menu(std::function<ftxui::Element(const ftxui::EntryState &state)>);
  void set_layout(const std::function<ftxui::Element()> preview);
  void set_input_handler(const std::function<bool(ftxui::Event)> handler,
                         const std::function<bool(ftxui::Event)> test);
  void set_deletion_dialog(const ftxui::Component deletion_dialog,
                           const std::function<bool(ftxui::Event)> handler);
  void move_selected_up(const int max);
  void move_selected_down(const int max);
  void toggle_deletion_dialog();
  void toggle_hidden_entries();
  void enter_direcotry(std::vector<std::string> curdir_entries_string);
  void leave_direcotry(std::vector<std::string> curdir_entries_string,
                       const int &previous_path_index);
  void
  update_curdir_entries_string(std::vector<std::string> curdir_entries_string);
  void update_entries_preview(ftxui::Element new_entries);
  ftxui::Element entries_preview();

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
