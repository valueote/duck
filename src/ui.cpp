#include "ui.hpp"
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
    : global_selected_{0}, view_selected_{0}, rename_cursor_positon_{0},
      active_pane_{0}, curdir_entries_{ftxui::emptyElement()},
      entries_preview_{ftxui::emptyElement()}, text_preview_{"Loading..."},
      screen_{ftxui::ScreenInteractive::FullscreenAlternateScreen()} {}

void Ui::set_main_layout(
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

void Ui::set_creation_dialog(
    ftxui::Component new_entry_dialog,
    std::function<bool(const ftxui::Event &)> handler) {
  creation_dialog_ = std::move(new_entry_dialog) | ftxui::CatchEvent(handler);
}

void Ui::set_notification(ftxui::Component notification) {
  notification_ = std::move(notification);
}

void Ui::finalize_tui() {
  auto components_tab = ftxui::Container::Tab(
      {
          main_layout_,
          deletion_dialog_,
          rename_dialog_,
          creation_dialog_,
          notification_,
      },
      &active_pane_);
  main_layout_->TakeFocus();

  tui_ = ftxui::Renderer(components_tab, [&] {
    auto main_ui_layer = main_layout_->Render();
    switch (active_pane_) {
    case static_cast<int>(pane::RENAME): {
      auto top_layer = ftxui::vbox({
          ftxui::vbox({}) |
              ftxui::size(ftxui::HEIGHT, ftxui::EQUAL,
                          (global_selected_ > 2 ? global_selected_ - 2
                                                : global_selected_ + 2)),
          ftxui::hbox({rename_dialog_->Render()}),
      });
      return ftxui::dbox({
          main_ui_layer,
          top_layer,
      });
    }
    case static_cast<int>(pane::DELETION): {
      return ftxui::dbox({
          main_ui_layer,
          ftxui::filler(),
          deletion_dialog_->Render() | ftxui::center,
      });
    }

    case static_cast<int>(pane::CREATION): {
      return ftxui::dbox({
          main_ui_layer,
          ftxui::filler(),
          creation_dialog_->Render() | ftxui::center,
      });
    }

    case static_cast<int>(pane::NOTIFICATION): {

      auto top_layer = ftxui::hbox(
          {ftxui::filler(), ftxui::vbox({
                                ftxui::hbox(notification_->Render()),
                                ftxui::filler(),
                            })});
      return ftxui::dbox({
          main_ui_layer,
          ftxui::filler(),
          top_layer,
      });
    }

    default:
      return main_ui_layer;
    }
  });
}

void Ui::move_selected_up(const int max) {
  if (max == 0) {
    return;
  }
  global_selected_ = (global_selected_ + max - 1) % max;
}

void Ui::move_selected_down(const int max) {
  if (max == 0) {
    return;
  }
  global_selected_ = (global_selected_ + 1) % max;
}

void Ui::toggle_deletion_dialog() {
  if (active_pane_ == static_cast<int>(pane::DELETION)) {
    active_pane_ = static_cast<int>(pane::MAIN);
    main_layout_->TakeFocus();
  } else if (active_pane_ == static_cast<int>(pane::MAIN)) {
    active_pane_ = static_cast<int>(pane::DELETION);
    deletion_dialog_->TakeFocus();
  }
}

void Ui::toggle_rename_dialog() {
  if (active_pane_ == static_cast<int>(pane::RENAME)) {
    active_pane_ = static_cast<int>(pane::MAIN);
    main_layout_->TakeFocus();
  } else if (active_pane_ == static_cast<int>(pane::MAIN)) {
    active_pane_ = static_cast<int>(pane::RENAME);
    rename_dialog_->TakeFocus();
  }
}

void Ui::toggle_creation_dialog() {
  if (active_pane_ == static_cast<int>(pane::CREATION)) {
    active_pane_ = static_cast<int>(pane::MAIN);
    main_layout_->TakeFocus();
  } else if (active_pane_ == static_cast<int>(pane::MAIN)) {
    active_pane_ = static_cast<int>(pane::CREATION);
    creation_dialog_->TakeFocus();
  }
}

void Ui::toggle_notification() {
  if (active_pane_ == static_cast<int>(pane::NOTIFICATION)) {
    active_pane_ = static_cast<int>(pane::MAIN);
  } else if (active_pane_ == static_cast<int>(pane::MAIN)) {
    active_pane_ = static_cast<int>(pane::NOTIFICATION);
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
  screen_.PostEvent(ftxui::Event::Custom);
}

void Ui::update_text_preview(std::string new_text_preview) {
  text_preview_ = std::move(new_text_preview);
  screen_.PostEvent(ftxui::Event::Custom);
}

void Ui::update_entries_preview(ftxui::Element new_entries) {
  entries_preview_ = std::move(new_entries);
  screen_.PostEvent(ftxui::Event::Custom);
}

void Ui::update_rename_input(std::string str) {
  rename_input_ = std::move(str);
  rename_cursor_positon_ = static_cast<int>(rename_input_.size());
}

void Ui::update_notification(std::string str) {
  notification_content_ = std::move(str);
}

std::vector<ftxui::Element> Ui::curdir_entries() { return curdir_entries_; }

ftxui::Element Ui::entries_preview() { return entries_preview_; }
std::string Ui::text_preview() { return text_preview_; }

std::string &Ui::rename_input() { return rename_input_; }

std::string &Ui::new_entry_input() { return new_entry_input_; }

std::string Ui::notification_content() { return notification_content_; }

int &Ui ::rename_cursor_positon() { return rename_cursor_positon_; }

void Ui::render() { screen_.Loop(tui_); }

void Ui::exit() { screen_.Exit(); }

int Ui::global_selected() const { return global_selected_; }

// Screen has a internal task queue which is protected by a mutex, so this
// opration is safe without lock;
void Ui::post_task(std::function<void()> task) { screen_.Post(task); }

void Ui::post_event(ftxui::Event event) { screen_.PostEvent(std::move(event)); }

void Ui::restored_io(std::function<void()> closure) {
  screen_.WithRestoredIO(std::move(closure));
}

} // namespace duck
