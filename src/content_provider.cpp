#include "content_provider.h"
#include "colorscheme.h"
#include "file_manager.h"
#include "ui.h"
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>

namespace duck {

ContentProvider::ContentProvider(Ui &ui, const ColorScheme &color_scheme)
    : ui_{ui}, color_scheme_{color_scheme} {}

std::function<ftxui::Element(const ftxui::EntryState &state)>
ContentProvider::menu_entries_transform() {
  return [this](const ftxui::EntryState &state) {
    auto style = state.active
                     ? ftxui::bgcolor(color_scheme_.selected()) | ftxui::bold |
                           ftxui::color(ftxui::Color::Black)
                     : ftxui::color(color_scheme_.text());
    return ftxui::text(state.label) | style;
  };
}

std::function<ftxui::Element()> ContentProvider::preview() {
  return [this]() {
    auto screen_size = ui_.screen_size();
    auto left_pane =
        window(
            ftxui::text(" " + FileManager::current_path().string() + " ") |
                ftxui::bold |
                ftxui::size(ftxui::WIDTH, ftxui::EQUAL, screen_size.first / 2),
            ui_.menu()->Render() | ftxui::frame) |
        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, screen_size.first / 2);

    auto right_pane =
        window(ftxui::text(" Content Preview ") | ftxui::bold,
               [this] {
                 const auto selected_path =
                     FileManager::selected_entry(ui_.selected());
                 if (not selected_path) {
                   return ftxui::text("No item selected");
                 }

                 if (fs::is_directory(selected_path.value())) {
                   return ui_.entries_preview() |
                          ftxui::color(color_scheme_.text());
                 }

                 return ftxui::paragraph(ui_.text_preview()) |
                        ftxui::color(color_scheme_.text()) |
                        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 80) |
                        ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 20);
               }() |
                   ftxui::vscroll_indicator | ftxui::frame) |
        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, screen_size.first / 2);

    return hbox(left_pane, ftxui::separator(), right_pane);
  };
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
  } else {
    auto selected_path = FileManager::selected_entry(ui_.selected());
    if (!selected_path.has_value()) {
      return ftxui::text("[ERROR] No file selected for deletion.");
    }
    return ftxui::vbox({
        ftxui::text(std::format("{}", selected_path->path().string())),
    });
  }
}

ftxui::Component ContentProvider::deletion_dialog() {
  ftxui::ButtonOption button_option;
  button_option.transform = [](const ftxui::EntryState &s) {
    auto style = s.active ? ftxui::bold : ftxui::nothing;
    return ftxui::text(s.label) | style | ftxui::center;
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
    auto screen_size = ui_.screen_size();
    auto dialog_content = ftxui::vbox(
        {deleted_entries() | ftxui::color(ftxui::Color::White), ftxui::filler(),
         ftxui::separator(),
         ftxui::hbox({
             ftxui::filler(),
             yes_button->Render(),
             ftxui::separatorEmpty() |
                 ftxui::size(ftxui::WIDTH, ftxui::EQUAL, screen_size.first / 3),
             no_button->Render(),
             ftxui::filler(),
         })});

    return ftxui::window(
               ftxui::vbox({ftxui::text("Permanently delete selected file?") |
                                ftxui::color(color_scheme_.warning()) |
                                ftxui::bold | ftxui::hcenter |
                                ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                                            screen_size.first / 3 * 2),
                            ftxui::filler()}),
               dialog_content) |
           ftxui::size(ftxui::WIDTH, ftxui::EQUAL, screen_size.first / 3 * 2) |
           ftxui::size(ftxui::HEIGHT, ftxui::EQUAL,
                       screen_size.second / 3 * 2) |
           ftxui::color(color_scheme_.border());
  });
  return dialog_renderer;
}

ftxui::Component ContentProvider::rename_dialog() {
  std::string content = "";

  ftxui::InputOption option;
  option.cursor_position = (int)content.size();
  auto input = ftxui::Input(&content, option);

  auto renderer = ftxui::Renderer(input, [&] {
    return ftxui::window(ftxui::text("Rename"), input->Render());
  });
  return renderer;
}

} // namespace duck
//
