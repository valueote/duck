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
#include <utility>
#include <vector>
namespace duck {

Ui::Ui()
    : global_selected_{0}, show_deletion_dialog_{false}, view_selected_{0},
      rename_cursor_positon_{0}, show_rename_dialog_{false}, active_pane_{0},
      curdir_entries_{ftxui::emptyElement()},
      entries_preview_{ftxui::emptyElement()}, text_preview_{"Loading..."},
      screen_{ftxui::ScreenInteractive::FullscreenAlternateScreen()} {}

void Ui::set_layout(
    ftxui::Component layout,
    std::function<bool(const ftxui::Event &)> navigation_handler,
    std::function<bool(const ftxui::Event &)> operation_handler) {
  main_layout_ = std::move(layout) | ftxui::CatchEvent(navigation_handler) |
                 ftxui::CatchEvent(operation_handler);
}

void Ui::set_deletion_dialog(
    ftxui::Component deletion_dialog,
    std::function<bool(const ftxui::Event &)> handler) {
  deletion_dialog_ =
      std::move(deletion_dialog) | ftxui::CatchEvent(handler) | ftxui::center;
}

void Ui::set_rename_dialog(ftxui::Component rename_dialog,
                           std::function<bool(const ftxui::Event &)> handler) {
  rename_dialog_ = std::move(rename_dialog) | ftxui::CatchEvent(handler);
}

void Ui::finalize_layout() {
  auto components_tab = ftxui::Container::Tab(
      {
          main_layout_,
          rename_dialog_,
          deletion_dialog_,
      },
      &active_pane_);
  main_layout_->TakeFocus();

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
    main_layout_->TakeFocus();
  }
}

void Ui::toggle_rename_dialog() {
  show_rename_dialog_ = !show_rename_dialog_;
  if (show_rename_dialog_) {
    active_pane_ = static_cast<int>(pane::RENAME);
    rename_dialog_->TakeFocus();
  } else {
    active_pane_ = static_cast<int>(pane::MAIN);
    main_layout_->TakeFocus();
  }
}

// FIX:: the previous_selected_ logic is uncorrect
void Ui::enter_direcotry(std::vector<ftxui::Element> curdir_entries) {
  if (previous_selected_.empty()) {
    global_selected_ = 0;
  } else {
    global_selected_ = previous_selected_.top();
    previous_selected_.pop();
  }
  update_curdir_entries(std::move(curdir_entries));
}

void Ui::leave_direcotry(std::vector<ftxui::Element> curdir_entries,
                         int previous_path_index) {
  previous_selected_.push(global_selected_);
  global_selected_ = previous_path_index;
  update_curdir_entries(std::move(curdir_entries));
}

void Ui::update_curdir_entries(std::vector<ftxui::Element> new_entries) {
  curdir_entries_ = std::move(new_entries);
  if (global_selected_ >= curdir_entries_.size()) {
    global_selected_ = 0;
  }
}

void Ui::update_entries_preview(ftxui::Element new_entries) {
  entries_preview_ = std::move(new_entries);
}

void Ui::update_rename_input(std::string str) {
  rename_input_ = std::move(str);
  rename_cursor_positon_ = static_cast<int>(rename_input_.size());
}

std::vector<ftxui::Element> Ui::curdir_entries() { return curdir_entries_; }

ftxui::Element Ui::entries_preview() { return entries_preview_; }

void Ui::update_text_preview(std::string new_text_preview) {
  text_preview_ = std::move(new_text_preview);
}
std::string Ui::text_preview() { return text_preview_; }

std::string &Ui::rename_input() { return rename_input_; }

int &Ui ::rename_cursor_positon() { return rename_cursor_positon_; }

void Ui::render() { screen_.Loop(tui_); }

void Ui::exit() { screen_.Exit(); }

int Ui::global_selected() const { return global_selected_; }

// Screen has a internal task queue which is protected by a mutex, so this
// opration is safe without holding ui_lock;
void Ui::post_task(std::function<void()> task) { screen_.Post(task); }

void Ui::post_event(ftxui::Event event) { screen_.PostEvent(std::move(event)); }

void Ui::restored_io(std::function<void()> closure) {
  screen_.WithRestoredIO(std::move(closure));
}

} // namespace duck
