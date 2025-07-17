#include "ui.h"
#include "filemanager.h"
#include "ftxui/dom/elements.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>
namespace duck {

UI::UI(FileManager &file_manager)
    : screen_{ftxui::ScreenInteractive::Fullscreen()},
      file_manager_(file_manager) {
  update_curdir_string_entires();
  build_menu();
  setup_layout();
}
UI::~UI() {}
void UI::build_menu() {
  menu_option_.on_change = [this]() { update_preview_content(); };
  FileManager &manager = this->file_manager_;
  menu_ = Menu(&curdir_string_entries_, &(manager.selected()), menu_option_) |
          ftxui::CatchEvent([this, &manager](ftxui::Event event) {
            if (event == ftxui::Event::Return ||
                event == ftxui::Event::Character('l')) {
              if (manager.get_selected_entry().has_value() &&
                  fs::is_directory(manager.get_selected_entry().value())) {
                manager.update_current_path(
                    fs::canonical(manager.get_selected_entry().value().path()));
                manager.update_curdir_entries();
                update_curdir_string_entires();
                manager.selected() = 0;
                menu_option_.on_change();
                build_menu();
              }
              return true;
            }
            if ((event == ftxui::Event::Backspace ||
                 event == ftxui::Event::Character('h'))) {
              std::cerr << "[Debug]" << manager.parent_path();
              manager.update_current_path(manager.parent_path());
              manager.update_curdir_entries();
              update_curdir_string_entires();
              manager.selected() = 0;
              menu_option_.on_change();
              build_menu();
              return true;
            }
            if (event == ftxui::Event::Character('q')) {
              screen_.Exit();
              return true;
            }
            return false;
          });
}

std::string UI::get_text_preview(const fs::path &path, size_t max_lines,
                                 size_t max_width) {

  if (fs::is_directory(path))
    return "[Directory]";
  if (!fs::is_regular_file(path))
    return "[Not a regular file]";
  std::ifstream file(path);
  if (!file)
    return "[Failed to open file]";

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

void UI::setup_layout() {
  FileManager &manager = this->file_manager_;
  layout_ = ftxui::Renderer(menu_, [&] {
    auto left_pane =
        window(ftxui::text(" " + manager.current_path().string() + " ") |
                   ftxui::bold,
               menu_->Render() | ftxui::vscroll_indicator | ftxui::frame |
                   ftxui::flex) |
        ftxui::flex;

    auto right_pane =
        window(ftxui::text(" Preview ") | ftxui::bold,
               ftxui::paragraph(get_text_preview(
                   manager.curdir_entries()[manager.selected()])) |
                   ftxui::vscroll_indicator | ftxui::frame | ftxui::flex) |
        ftxui::flex;

    return hbox(left_pane, ftxui::separator(), right_pane);
  });
}
void UI::update_curdir_string_entires() {
  curdir_string_entries_ =
      file_manager_.curdir_entries() |
      std::views::transform([this](const fs::directory_entry &entry) {
        return this->format_directory_entries(entry);
      }) |
      std::ranges::to<std::vector>();
}

void UI::update_preview_content() {
  if (file_manager_.curdir_entries().empty()) {
    file_preview_content_ = "[Empty directory]";
    return;
  }

  const auto &entry = file_manager_.curdir_entries()[file_manager_.selected()];
  file_preview_content_ = get_text_preview(entry.path());

  // if (fs::is_directory(entry)) {
  //   file_manager_.load_directory_entries(entry.path(), dir_preview_content_);
  // } else {
  //   file_preview_content_ = get_text_preview(entry.path());
  // }
}

void UI::render() { screen_.Loop(layout_); }

std::string UI::format_directory_entries(const fs::directory_entry &entry) {
  static const std::unordered_map<std::string, std::string> extension_icons{
      {".txt", "\uf15c"}, {".md", "\ueeab"},   {".cpp", "\ue61d"},
      {".hpp", "\uf0fd"}, {".h", "\uf0fd"},    {".c", "\ue61e"},
      {".jpg", "\uf4e5"}, {".jpeg", "\uf4e5"}, {".png", "\uf4e5"},
      {".gif", "\ue60d"}, {".pdf", "\ue67d"},  {".zip", "\ue6aa"},
      {".mp3", "\uf001"}, {".mp4", "\uf03d"},  {".json", "\ue60b"},
      {".log", "\uf4ed"}, {".csv", "\ueefc"},
  };

  const auto filename = entry.path().filename().string();

  if (fs::is_directory(entry)) {
    return std::format("\uf4d3 {}", filename);
  }

  auto ext = entry.path().extension().string();
  std::ranges::transform(ext, ext.begin(), [](char c) {
    return static_cast<char>(std::tolower(c));
  });

  auto icon_it = extension_icons.find(ext);
  const std::string &icon =
      icon_it != extension_icons.end() ? icon_it->second : "\uf15c";

  return std::format("{} {}", icon, filename);
}

} // namespace duck
