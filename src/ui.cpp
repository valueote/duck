#include "ui.hpp"
#include "app_event.hpp"
#include "app_state.hpp"
#include "ftxui/dom/elements.hpp"
#include "input_handler.hpp"
#include <cstddef>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>
#include <unistd.h>
#include <utility>

namespace duck {
using std::string;

Ui::Ui(InputHandler &input_handler)
    : input_handler_{input_handler}, cursor_positon_{0}, active_pane_{0},
      screen_{ftxui::ScreenInteractive::FullscreenAlternateScreen()} {

  selected_entries_ = ftxui::text("");

  rename_dialog_ =
      content_provider_.rename_dialog(cursor_positon_, input_content_) |
      ftxui::CatchEvent(input_handler_.rename_dialog_handler());

  creation_dialog_ =
      content_provider_.creation_dialog(cursor_positon_, input_content_) |
      ftxui::CatchEvent(input_handler_.creation_dialog_handler());

  deletion_dialog_ =
      content_provider_.deletion_dialog(
          [this]() { return selected_entries_; },
          [this]() { screen_.PostEvent(ftxui::Event::Character('y')); },
          [this]() { screen_.PostEvent(ftxui::Event::Character('n')); }) |
      ftxui::CatchEvent(input_handler_.deletion_dialog_handler());

  notification_ = content_provider_.notification(notification_content_);

  main_layout_ = content_provider_.layout(info_, preview_) |
                 ftxui::CatchEvent(input_handler_.navigation_handler()) |
                 ftxui::CatchEvent(input_handler_.operation_handler());

  finalize_tui();
}

void Ui::async_update_info(MenuInfo new_info) {
  screen_.Post([this, info = std::move(new_info)]() { info_ = info; });
  screen_.PostEvent(ftxui::Event::Custom);
};

void Ui::update_whole_state(const AppState &state) {}

void Ui::async_update_index(size_t index) {
  screen_.Post([this, index]() { std::get<1>(info_) = index; });
  screen_.PostEvent(ftxui::Event::Custom);
}

void Ui::async_update_selected(ftxui::Element selected_entries) {
  screen_.Post([this, entries = std::move(selected_entries)]() {
    selected_entries_ = entries;
  });
  screen_.PostEvent(ftxui::Event::Custom);
}

void Ui::async_update_preview(EntryPreview new_preview) {
  screen_.Post(
      [this, preview = std::move(new_preview)]() { preview_ = preview; });
  screen_.PostEvent(ftxui::Event::Custom);
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
  screen_.Post([this]() {
    if (active_pane_ == static_cast<int>(pane::DELETION)) {
      active_pane_ = static_cast<int>(pane::MAIN);
      // main_layout_->TakeFocus();
    } else if (active_pane_ == static_cast<int>(pane::MAIN)) {
      active_pane_ = static_cast<int>(pane::DELETION);
      // deletion_dialog_->TakeFocus();
    }
  });
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

void Ui::update_rename_input(string input) {
  screen_.Post([input, this]() {
    input_content_ = input;
    cursor_positon_ = static_cast<int>(input_content_.size());
  });
}

void Ui::update_notification(std::string input) {
  screen_.Post([this, input]() { notification_content_ = input; });
}

std::string &Ui::input_content() { return input_content_; }

int &Ui ::cursor_positon() { return cursor_positon_; }

void Ui::render(AppState &state) {
  info_ = {state.current_directory_.path_.string(), state.index_,
           state.current_directory_elements()};
  preview_ = ftxui::text("Loading");
  selected_entries_ = ftxui::text("Nothing");
  screen_.Loop(tui_);
}

void Ui::exit() { screen_.Exit(); }

void Ui::post_task(std::function<void()> task) { screen_.Post(task); }

void Ui::restored_io(std::function<void()> closure) {
  screen_.WithRestoredIO(std::move(closure));
}

} // namespace duck
