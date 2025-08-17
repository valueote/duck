#include "ui.h"
#include "ftxui/dom/elements.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>
#include <unistd.h>
#include <vector>
namespace duck {

Ui::Ui()
    : global_selected_{0}, show_deletion_dialog_{false},
      show_rename_dialog_{false}, entries_preview_{ftxui::emptyElement()},
      text_preview_{"Loading..."},
      screen_{ftxui::ScreenInteractive::Fullscreen()} {}

void Ui::set_menu(
    const std::function<ftxui::Element(const ftxui::EntryState &state)>
        entries_transform) {
  menu_option_.focused_entry = &global_selected_;
  menu_option_.entries_option.transform = entries_transform;
  menu_ = Menu(&curdir_string_entries_, &(global_selected_), menu_option_);
}

void Ui::set_input_handler(
    const std::function<bool(const ftxui::Event &)> handler) {
  menu_ = menu_ | ftxui::CatchEvent(handler);
}

void Ui::set_layout(const std::function<ftxui::Element()> preview) {
  main_layout_ = ftxui::Renderer(menu_, preview);
}

void Ui::set_deletion_dialog(
    const ftxui::Component deletion_dialog,
    const std::function<bool(const ftxui::Event &)> handler) {
  deletion_dialog_ =
      deletion_dialog | ftxui::CatchEvent(handler) | ftxui::center;
}

void Ui::set_rename_dialog(
    const ftxui::Component rename_dialog,
    const std::function<bool(const ftxui::Event &)> handler) {
  rename_dialog_ = rename_dialog | ftxui::CatchEvent(handler);
}

void Ui::finalize_layout() {
  auto components_tab = ftxui::Container::Tab(
      {
          main_layout_,
          rename_dialog_,
          deletion_dialog_,
      },
      &active_pane_);

  tui_ = ftxui::Renderer(components_tab, [&] {
    auto main_ui_layer = main_layout_->Render();

    if (show_rename_dialog_) {
      auto dialog_element = rename_dialog_->Render();

      auto top_layer = ftxui::vbox({
          ftxui::vbox({}) |
              ftxui::size(ftxui::HEIGHT, ftxui::EQUAL,
                          (global_selected_ > 2 ? global_selected_ - 2
                                                : global_selected_ + 2)),
          ftxui::hbox({
              dialog_element,
          }),
      });
      return ftxui::dbox({
          main_ui_layer,
          top_layer,
      });
    }

    if (show_deletion_dialog_) {
      return ftxui::dbox({
          main_ui_layer,
          ftxui::filler(),
          deletion_dialog_->Render() | ftxui::center,
      });
    }

    return main_ui_layer;
  });
}

// move selected up and down can only be used in ui thread
void Ui::move_selected_up(const int max) {
  if (global_selected_ > 0) {
    global_selected_--;
  } else {
    global_selected_ = max;
  }
}

void Ui::move_selected_down(const int max) {
  if (global_selected_ < max) {
    global_selected_++;
  } else {
    global_selected_ = 0;
  }
}

void Ui::toggle_deletion_dialog() {
  show_deletion_dialog_ = !show_deletion_dialog_;
  if (show_deletion_dialog_) {
    active_pane_ = static_cast<int>(pane::DELETION);
    deletion_dialog_->TakeFocus();
  } else {
    active_pane_ = static_cast<int>(pane::MAIN);
  }
}

void Ui::toggle_rename_dialog() {
  show_rename_dialog_ = !show_rename_dialog_;
  if (show_rename_dialog_) {
    active_pane_ = static_cast<int>(pane::RENAME);
    rename_dialog_->TakeFocus();
  } else {
    active_pane_ = static_cast<int>(pane::MAIN);
  }
}

void Ui::enter_direcotry(std::vector<std::string> curdir_entries_string) {
  update_curdir_entries_string(std::move(curdir_entries_string));
  if (previous_selected_.empty()) {
    global_selected_ = 0;
  } else {
    global_selected_ = previous_selected_.top();
    previous_selected_.pop();
  }
}

void Ui::leave_direcotry(std::vector<std::string> curdir_entries_string,
                         const int &previous_path_index) {
  update_curdir_entries_string(std::move(curdir_entries_string));
  previous_selected_.push(global_selected_);
  global_selected_ = previous_path_index;
}

void Ui::update_curdir_entries_string(
    std::vector<std::string> curdir_entries_string) {
  curdir_string_entries_ = std::move(curdir_entries_string);
}

void Ui::update_entries_preview(ftxui::Element new_entries) {
  entries_preview_ = std::move(new_entries);
}

void Ui::update_rename_input(std::string str) {
  rename_input_ = std::move(str);
  rename_cursor_positon_ = rename_input_.size();
}

ftxui::Element Ui::entries_preview() { return entries_preview_; }

void Ui::update_text_preview(std::string new_text_preview) {
  text_preview_ = std::move(new_text_preview);
}
std::string Ui::text_preview() { return text_preview_; }

std::string &Ui::rename_input() { return rename_input_; }

int &Ui ::rename_cursor_positon() { return rename_cursor_positon_; }

void Ui::render() { screen_.Loop(tui_); }

void Ui::exit() { screen_.Exit(); }

int Ui::selected() { return global_selected_; }

// Screen has a internal task queue which is protected by a mutex, so this
// opration is safe without holding ui_lock;
void Ui::post_task(std::function<void()> task) { screen_.Post(task); }

std::pair<int, int> Ui::screen_size() {
  return {screen_.dimx(), screen_.dimy()};
}
void Ui::post_event(ftxui::Event event) { screen_.PostEvent(std::move(event)); }

void Ui::restored_io(const std::function<void()> closure) {
  screen_.WithRestoredIO(closure);
}

ftxui::Component &Ui::menu() { return menu_; }

} // namespace duck
