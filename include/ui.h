#pragma once
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
#include <vector>

namespace duck {

class Ui {
private:
  std::shared_mutex ui_lock_;
  std::string text_preview_;

  std::vector<ftxui::Element> curdir_entries_;
  ftxui::Element entries_preview_;

  ftxui::ScreenInteractive screen_;
  ftxui::Component main_layout_;
  ftxui::Component tui_;
  ftxui::Component deletion_dialog_;

  ftxui::Component creation_dialog_;
  std::string new_entry_input_;

  ftxui::Component rename_dialog_;
  std::string rename_input_;
  int rename_cursor_positon_;

  std::stack<int> previous_selected_;
  int global_selected_;
  int view_selected_;

  enum class pane : int8_t { MAIN = 0, DELETION, RENAME, CREATION };
  int active_pane_;

public:
  Ui();
  void set_layout(ftxui::Component layout,
                  std::function<bool(const ftxui::Event &)> navigation_handler,
                  std::function<bool(const ftxui::Event &)> operation_handler);
  void set_deletion_dialog(ftxui::Component deletion_dialog,
                           std::function<bool(const ftxui::Event &)> handler);

  void set_rename_dialog(ftxui::Component rename_dialog,
                         std::function<bool(const ftxui::Event &)> handler);

  void set_creation_dialog(ftxui::Component new_entry_dialog,
                           std::function<bool(const ftxui::Event &)> handler);
  void finalize_layout();

  void move_selected_up(int max);
  void move_selected_down(int max);
  void toggle_deletion_dialog();
  void toggle_rename_dialog();
  void toggle_creation_dialog();

  void enter_direcotry(std::vector<ftxui::Element> curdir_entries);
  void leave_direcotry(std::vector<ftxui::Element> curdir_entries,
                       int previous_path_index);
  void update_entries_preview(ftxui::Element new_entries);
  void update_curdir_entries(std::vector<ftxui::Element> new_entries);
  void update_rename_input(std::string str);

  std::vector<ftxui::Element> curdir_entries();
  ftxui::Element entries_preview();
  void update_text_preview(std::string new_text_preview);
  std::string text_preview();
  std::string &rename_input();
  std::string &new_entry_input();
  int &rename_cursor_positon();

  void render();
  void exit();
  [[nodiscard]] int global_selected() const;
  bool show_hidden();
  void post_event(ftxui::Event event);
  void post_task(std::function<void()> task);
  void restored_io(std::function<void()> closure);
};

} // namespace duck
