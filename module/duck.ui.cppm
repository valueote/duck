module;
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

export module duck.ui;

namespace duck {

export class Ui {
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
  bool show_deletion_dialog_;

public:
  Ui();
  void set_menu(std::function<ftxui::Element(const ftxui::EntryState &state)>);
  void set_layout(const std::function<ftxui::Element()> preview);
  void set_input_handler(const std::function<bool(ftxui::Event)> handler);
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

  void render();
  void exit();
  int selected();
  bool show_hidden();
  void post_event(const ftxui::Event &event);
  void restored_io(const std::function<void()> closure);
  std::pair<int, int> screen_size();
  ftxui::Component &menu();
};

} // namespace duck
