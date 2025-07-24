#pragma once
#include "colorscheme.h"
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
  std::vector<std::string> curdir_string_entries_;
  std::vector<std::string> previewdir_string_entries_;
  std::string file_preview_content_;
  std::vector<std::string> dir_preview_content_;

  ftxui::ScreenInteractive screen_;
  ftxui::Component main_layout_;
  ftxui::MenuOption menu_option_;
  ftxui::Component modal_;
  ftxui::Component menu_;

  std::stack<int> previous_selected_;
  int selected_;
  bool show_delete_dialog_;

  const ColorScheme &color_scheme_;

public:
  Ui(const ColorScheme &color_scheme);

  void set_layout(const std::function<ftxui::Element()> preview);
  void set_input_handler(const std::function<bool(ftxui::Event)> handler);
  void set_deletion_dialog(const std::function<ftxui::Element()> deleted_entry,
                           const std::function<bool(ftxui::Event)> handler);
  void move_selected_up(const int max);
  void move_selected_down(const int max);
  void toggle_delete_dialog();
  void enter_direcotry(std::vector<std::string> curdir_entries_string);
  void leave_direcotry(std::vector<std::string> curdir_entries_string,
                       const int &previous_path_index);
  void
  update_curdir_string_entires(std::vector<std::string> curdir_entries_string);

  void render();
  void exit();
  int selected();
  ftxui::Component &menu();
  ftxui::ScreenInteractive &screen();
};

} // namespace duck
