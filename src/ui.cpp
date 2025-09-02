#include "ui.hpp"
#include "app_state.hpp"
#include "ftxui/dom/elements.hpp"
#include "input_handler.hpp"
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

Ui::Ui(InputHandler &input_handler)
    : input_handler_{input_handler}, cursor_positon_{0}, active_pane_{0},
      curdir_entries_{ftxui::text("test")},
      screen_{ftxui::ScreenInteractive::FullscreenAlternateScreen()} {
  notification_ = content_provider_.notification(notification_content_);
  rename_dialog_ =
      content_provider_.rename_dialog(cursor_positon_, input_content_) |
      ftxui::CatchEvent(input_handler_.rename_dialog_handler());

  creation_dialog_ =
      content_provider_.creation_dialog(cursor_positon_, input_content_) |
      ftxui::CatchEvent(input_handler_.creation_dialog_handler());
}

void Ui::set_main_layout(const AppState &state) {
  main_layout_ = content_provider_.layout(state, curdir_entries_) |
                 ftxui::CatchEvent(input_handler_.navigation_handler()) |
                 ftxui::CatchEvent(input_handler_.operation_handler());
}

void Ui::set_deletion_dialog(const AppState &state) {
  deletion_dialog_ =
      content_provider_.deletion_dialog(
          state, []() {}, []() {}) |
      ftxui::CatchEvent(input_handler_.deletion_dialog_handler());
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

  tui_ = ftxui::Renderer(components_tab, [this] {
    auto main_ui_layer = main_layout_->Render();
    switch (active_pane_) {
    case static_cast<int>(pane::RENAME): {
      auto top_layer = ftxui::vbox({
          ftxui::vbox({}) |
              ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, (0 > 2 ? 0 - 2 : 0 + 2)),
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

void Ui::enter_direcotry(AppState &state,
                         std::vector<ftxui::Element> curdir_entries) {
  if (index_selected_.empty()) {
    state.index = 0;
  } else {
    state.index = index_selected_.top();
    index_selected_.pop();
    if (state.index >= curdir_entries_.size()) {
      state.index = 0;
    }
  }
  update_curdir_entries(std::move(curdir_entries));
}

void Ui::leave_direcotry(AppState &state,
                         std::vector<ftxui::Element> curdir_entries,
                         int previous_path_index) {
  index_selected_.push(state.index);
  state.index = previous_path_index;
  update_curdir_entries(std::move(curdir_entries));
}

void Ui::update_curdir_entries(std::vector<ftxui::Element> new_entries) {
  curdir_entries_ = std::move(new_entries);
  screen_.PostEvent(ftxui::Event::Custom);
}

void Ui::update_rename_input(std::string str) {
  input_content_ = std::move(str);
  cursor_positon_ = static_cast<int>(input_content_.size());
}

void Ui::update_notification(std::string str) {
  notification_content_ = std::move(str);
}

std::string &Ui::input_content() { return input_content_; }

std::string Ui::notification_content() { return notification_content_; }

int &Ui ::cursor_positon() { return cursor_positon_; }

void Ui::render(const AppState &state) {
  set_main_layout(state);
  set_deletion_dialog(state);
  finalize_tui();
  screen_.Loop(tui_);
}

void Ui::exit() { screen_.Exit(); }

void Ui::post_task(std::function<void()> task) { screen_.Post(task); }

void Ui::restored_io(std::function<void()> closure) {
  screen_.WithRestoredIO(std::move(closure));
}

} // namespace duck
