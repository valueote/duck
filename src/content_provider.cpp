#include "content_provider.h"
#include "colorscheme.h"
#include "ui.h"
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>

namespace duck {

ContentProvider::ContentProvider(FileManager &file_manager, Ui &ui,
                                 const ColorScheme &color_scheme)
    : file_manager_{file_manager}, ui_{ui}, color_scheme_{color_scheme} {}

std::function<ftxui::Element(const ftxui::EntryState &state)>
ContentProvider::entries_transform() {
  return [this](const ftxui::EntryState &state) {
    auto style = state.active
                     ? ftxui::bgcolor(color_scheme_.selected()) | ftxui::bold |
                           ftxui::color(ftxui::Color::Black)
                     : ftxui::color(color_scheme_.text());
    return ftxui::text(state.label) | style;
  };
}

std::function<ftxui::Element()> ContentProvider::preview_async() {
  return [this]() {
    auto screen_size = ui_.screen_size();
    auto left_pane =
        window(ftxui::text(" " + file_manager_.current_path().string() + " ") |
                   ftxui::bold,
               ui_.menu()->Render() | ftxui::vscroll_indicator | ftxui::frame) |
        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, screen_size.first / 2);

    auto right_pane =
        window(ftxui::text(" Coneten Preview ") | ftxui::bold,
               [this] {
                 const auto selected_path =
                     file_manager_.get_selected_entry(ui_.selected());
                 if (not selected_path) {
                   return ftxui::text("No item selected");
                 }

                 if (fs::is_directory(selected_path.value())) {
                   auto task =
                       get_directory_preview_async(selected_path.value());
                   stdexec::start_detached(std::move(task));
                   return ui_.entries_preview() |
                          ftxui::color(color_scheme_.text());
                 }
                 auto task = get_text_preview_async(selected_path.value());
                 stdexec::start_detached(std::move(task));
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
  if (!file_manager_.marked_entries().empty()) {
    std::vector<ftxui::Element> lines =
        file_manager_.marked_entries() |
        std::views::transform([this](const fs::directory_entry &entry) {
          return ftxui::text(file_manager_.entry_name_with_icon(entry));
        }) |
        std::ranges::to<std::vector>();
    return ftxui::vbox({lines});
  } else {
    auto selected_path = file_manager_.get_selected_entry(ui_.selected());
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

std::string ContentProvider::get_text_preview(const fs::path &path,
                                              size_t max_lines,
                                              size_t max_width) {
  std::ifstream file(path);

  std::ostringstream oss;
  std::string line;
  size_t lines = 0;

  while (std::getline(file, line) && lines < max_lines) {

    if (line.length() > max_width) {
      line = line.substr(0, max_width - 3) + "...";
    }

    for (auto &c : line) {
      if (iscntrl(static_cast<unsigned char>(c))) {
        c = '?';
      }
    }

    oss << line << '\n';
    ++lines;
  }
  return oss.str();
}

} // namespace duck
