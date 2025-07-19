#include "ui.h"
#include "filemanager.h"
#include "ftxui/dom/elements.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <ranges>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
namespace duck {

// TODO: Add a parent dir plane
UI::UI()
    : selected_{0}, previous_selected_(0),
      screen_{ftxui::ScreenInteractive::Fullscreen()} {
  menu_option_.focused_entry = &selected_;
  menu_ = Menu(&curdir_string_entries_, &(selected_), menu_option_);
}

void UI::set_input_handler(std::function<bool(ftxui::Event)> handler) {
  menu_ = menu_ | ftxui::CatchEvent(handler);
}

void UI::setup_layout(std::function<ftxui::Element()> layout_builder) {
  layout_ = ftxui::Renderer(menu_, layout_builder);
}

void UI::move_down_direcotry(FileManager &file_manager) {
  update_curdir_string_entires(file_manager);
  selected_ = previous_selected_;
}

void UI::move_up_direcotry(FileManager &file_manager) {
  update_curdir_string_entires(file_manager);
  previous_selected_ = selected_;
}

void UI::set_selected_previous_dir(FileManager &file_manager) {
  const auto &entries = file_manager.curdir_entries();
  const auto &previous = file_manager.previous_path();

  if (auto it = std::ranges::find(entries, previous); it != entries.end()) {
    selected_ = static_cast<int>(std::distance(entries.begin(), it));
  }
}

void UI::update_curdir_string_entires(FileManager &file_manager) {
  curdir_string_entries_ =
      file_manager.curdir_entries() |
      std::views::transform([this](const fs::directory_entry &entry) {
        return this->format_directory_entries(entry);
      }) |
      std::ranges::to<std::vector>();
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

int UI::get_selected() { return selected_; }

ftxui::Component &UI::get_menu() { return menu_; }

ftxui::ScreenInteractive &UI::get_screen() { return screen_; };

void UI::exit() { screen_.Exit(); }
} // namespace duck
