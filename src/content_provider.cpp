#include "content_provider.h"
#include "colorscheme.h"
#include "file_manager.h"
#include "ui.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>

namespace duck {

ContentProvider::ContentProvider(Ui &ui) : ui_{ui} {}

std::function<ftxui::Element(const ftxui::EntryState &state)>
ContentProvider::menu_entries_transform() {
  return [this](const ftxui::EntryState &state) {
    auto style = state.active
                     ? ftxui::bgcolor(ColorScheme::selected()) | ftxui::bold |
                           ftxui::color(ftxui::Color::Black)
                     : ftxui::color(ColorScheme::text());
    return ftxui::text(state.label) | style;
  };
}

ftxui::Element ContentProvider::visible_entries() {
  auto [width, _] = ftxui::Terminal::Size();
  auto all_entries = ui_.curdir_entries();
  if (all_entries.empty()) {
    return window(
               ftxui::text(" " + FileManager::current_path().string() + " ") |
                   ftxui::bold |
                   ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, width / 2),
               ftxui::vbox({ftxui::text("[Empty directory]")})) |
           ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width / 2);
  }

  int selected = ui_.global_selected();
  std::vector<ftxui::Element> visible_entries;
  int selected_in_view = 0;
  // delete the border and margin space
  const int viewport_size = ftxui::Terminal::Size().dimy - 2;

  if (all_entries.size() > viewport_size) {
    int start_index = selected - (viewport_size / 4 * 3);
    start_index = std::max(0, start_index);

    if (start_index + viewport_size > all_entries.size()) {
      start_index = static_cast<int>(all_entries.size()) - viewport_size;
    }

    int end_index = start_index + viewport_size;
    selected_in_view = selected - start_index;

    visible_entries.assign(
        std::make_move_iterator(all_entries.begin() + start_index),
        std::make_move_iterator(all_entries.begin() + end_index));
  } else {
    visible_entries = all_entries;
    selected_in_view = selected;
  }
  visible_entries[selected_in_view] |= ftxui::color(ftxui::Color::Black) |
                                       ftxui::bgcolor(ColorScheme::selected());
  return ftxui::vbox(std::move(visible_entries));
}

ftxui::Element ContentProvider::left_pane() {
  auto [width, _] = ftxui::Terminal::Size();
  auto pane =
      window(ftxui::text(" " + FileManager::current_path().string() + " ") |
                 ftxui::bold |
                 ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, width / 2),
             visible_entries()) |
      ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width / 2);

  return pane;
}

ftxui::Element ContentProvider::right_pane() {
  auto [width, _] = ftxui::Terminal::Size();
  auto pane = window(ftxui::text(" Content Preview ") | ftxui::bold,
                     [this] {
                       const auto selected_path =
                           FileManager::selected_entry(ui_.global_selected());
                       if (not selected_path) {
                         return ftxui::text("No item selected");
                       }

                       if (fs::is_directory(selected_path.value())) {
                         return ui_.entries_preview() |
                                ftxui::color(ColorScheme::text());
                       }

                       return ftxui::paragraph(ui_.text_preview()) |
                              ftxui::color(ColorScheme::text());
                     }()) |
              ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width / 2);
  return pane;
}

ftxui::Component ContentProvider::layout() {
  auto dummy = ftxui::Button({});
  auto render = ftxui::Renderer(dummy, [this]() {
    return ftxui::hbox(left_pane(), ftxui::separator(), right_pane());
  });
  return render;
}

ftxui::Element ContentProvider::deleted_entries() {
  if (!FileManager::marked_entries().empty()) {
    std::vector<ftxui::Element> lines =
        FileManager::marked_entries() |
        std::views::transform([this](const fs::directory_entry &entry) {
          return ftxui::text(FileManager::entry_name_with_icon(entry));
        }) |
        std::ranges::to<std::vector>();
    return ftxui::vbox({lines});
  }

  auto selected_path = FileManager::selected_entry(ui_.global_selected());
  if (!selected_path.has_value()) {
    return ftxui::text("[ERROR] No file selected for deletion.");
  }

  return ftxui::vbox({
      ftxui::text(std::format("{}", selected_path->path().string())),
  });
}

ftxui::Component ContentProvider::deletion_dialog() {
  ftxui::ButtonOption button_option;
  button_option.transform = [](const ftxui::EntryState &state) {
    auto style = state.active ? ftxui::bold : ftxui::nothing;
    return ftxui::text(state.label) | style | ftxui::center;
  };

  auto yes_button = ftxui::Button(
      "[Y]es", [this] { ui_.post_event(ftxui::Event::Character('y')); },
      button_option);

  auto no_button = ftxui::Button(
      "[N]o", [this] { ui_.post_event(ftxui::Event::Character('n')); },
      button_option);
  auto button_container = ftxui::Container::Horizontal({yes_button, no_button});
  auto dialog_renderer = ftxui::Renderer(button_container, [yes_button,
                                                            no_button, this] {
    auto [width, height] = ftxui::Terminal::Size();
    auto dialog_content =
        ftxui::vbox({deleted_entries() | ftxui::color(ftxui::Color::White),
                     ftxui::filler(), ftxui::separator(),
                     ftxui::hbox({
                         ftxui::filler(),
                         yes_button->Render(),
                         ftxui::separatorEmpty() |
                             ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width / 3),
                         no_button->Render(),
                         ftxui::filler(),
                     })});

    return ftxui::window(
               ftxui::vbox(
                   {ftxui::text("Permanently delete selected file?") |
                        ftxui::color(ColorScheme::warning()) | ftxui::bold |
                        ftxui::hcenter |
                        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width / 3 * 2),
                    ftxui::filler()}),
               dialog_content) |
           ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width / 3 * 2) |
           ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height / 3 * 2) |
           ftxui::color(ColorScheme::border()) | ftxui::clear_under;
    ;
  });
  return dialog_renderer;
}

ftxui::Component ContentProvider::rename_dialog() {

  ftxui::InputOption option = ftxui::InputOption::Default();
  option.cursor_position = &ui_.rename_cursor_positon();
  option.transform = [this](ftxui::InputState state) -> ftxui::Element {
    state.element |= color(ColorScheme::text());
    return state.element;
  };

  auto input = ftxui::Input(&ui_.rename_input(), option);
  auto renderer = ftxui::Renderer(input, [this, input] {
    const auto [width, height] = ftxui::Terminal::Size();
    auto dialog = ftxui::window(ftxui::text("Rename"), input->Render()) |
                  ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width / 2) |
                  ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 3) |
                  ftxui::clear_under;
    ;
    return dialog;
  });

  return renderer;
}

ftxui::Component ContentProvider::creation_dialog() {
  ftxui::InputOption option = ftxui::InputOption::Default();
  option.cursor_position = &ui_.rename_cursor_positon();
  option.transform = [this](ftxui::InputState state) -> ftxui::Element {
    state.element |= color(ColorScheme::text());
    return state.element;
  };

  auto input = ftxui::Input(&ui_.new_entry_input(), option);
  auto renderer = ftxui::Renderer(input, [this, input] {
    const auto [width, _] = ftxui::Terminal::Size();
    auto dialog = ftxui::window(ftxui::text("Create"), input->Render()) |
                  ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width / 2) |
                  ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 3) |
                  ftxui::clear_under;
    ;
    return dialog;
  });

  return renderer;
}

ftxui::Component ContentProvider::notification() {
  return ftxui::Renderer([this]() {
    auto [width, _] = ftxui::Terminal::Size();
    return ftxui::window(ftxui::text("notification"),
                         ftxui::text(ui_.notification_content())) |
           ftxui::flex | ftxui::clear_under;
  });
}

} // namespace duck
//
