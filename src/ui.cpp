#include "ui.h"
#include "ftxui/dom/elements.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <print>
#include <ranges>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
namespace duck {

// TODO: Add a parent dir plane
UI::UI()
    : selected_{0}, previous_selected_{0}, show_delete_dialog_{false},
      screen_{ftxui::ScreenInteractive::Fullscreen()} {
  menu_option_.focused_entry = &selected_;
  menu_ = Menu(&curdir_string_entries_, &(selected_), menu_option_);
}

void UI::set_input_handler(const std::function<bool(ftxui::Event)> handler) {
  menu_ = menu_ | ftxui::CatchEvent(handler);
}

void UI::set_layout(const std::function<ftxui::Element()> layout_builder) {
  layout_ = ftxui::Renderer(menu_, layout_builder);
}

void UI::set_delete_dialog(std::function<bool(ftxui::Event)> handler) {
  auto on_confirm = [this] { show_delete_dialog_ = false; };

  auto on_cancel = [this] { show_delete_dialog_ = false; };

  auto yes_button = ftxui::Button("[Y]es", on_confirm);
  auto no_button = ftxui::Button("[N]o", on_cancel);

  auto button_row = ftxui::Container::Horizontal({yes_button, no_button});

  button_row |= ftxui::CatchEvent(handler);

  auto dialog_content = ftxui::Renderer(button_row, [=] {
    return ftxui::vbox({ftxui::text("Trash  selected file?"),
                        ftxui::separator(),
                        ftxui::text("/home/vivy/text_relax_output.txt"),
                        ftxui::hbox({
                            yes_button->Render(),
                            ftxui::filler(),
                            no_button->Render(),
                        })}) |
           ftxui::border;
  });

  modal_ = ftxui::Modal(layout_, dialog_content, &show_delete_dialog_);
}

void UI::toggle_delete_dialog() { show_delete_dialog_ = !show_delete_dialog_; }

void UI::enter_direcotry(
    const std::vector<fs::directory_entry> &curdir_entries) {
  update_curdir_string_entires(curdir_entries);
  selected_ = previous_selected_;
}

void UI::leave_direcotry(const std::vector<fs::directory_entry> &curdir_entries,
                         const fs::path &previous_path) {
  update_curdir_string_entires(curdir_entries);
  previous_selected_ = selected_;
  if (auto it = std::ranges::find(curdir_entries, previous_path);
      it != curdir_entries.end()) {
    selected_ = static_cast<int>(std::distance(curdir_entries.begin(), it));
  } else {
    std::print(stderr, "[ERROR]: error in leave_direcotry");
  }
}

void UI::update_curdir_string_entires(
    const std::vector<fs::directory_entry> &curdir_entries) {
  curdir_string_entries_ =
      curdir_entries |
      std::views::transform([this](const fs::directory_entry &entry) {
        return this->format_directory_entries(entry);
      }) |
      std::ranges::to<std::vector>();
}

const std::string
UI::format_directory_entries(const fs::directory_entry &entry) {
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

void UI::render() { screen_.Loop(modal_); }

void UI::exit() { screen_.Exit(); }

int UI::selected() { return selected_; }

ftxui::Component &UI::menu() { return menu_; }

ftxui::ScreenInteractive &UI::screen() { return screen_; };

} // namespace duck
