#include "ui.h"
#include "ftxui/dom/elements.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
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
Ui::Ui()
    : selected_{0}, previous_selected_{0}, show_delete_dialog_{false},
      menu_option_{.focused_entry = &selected_},
      menu_{Menu(&curdir_string_entries_, &(selected_), menu_option_)},
      screen_{ftxui::ScreenInteractive::Fullscreen()} {}

void Ui::set_input_handler(const std::function<bool(ftxui::Event)> handler) {
  menu_ = menu_ | ftxui::CatchEvent(handler);
}

void Ui::set_layout(const std::function<ftxui::Element()> preview) {
  main_layout_ = ftxui::Renderer(menu_, preview);
}

void Ui::set_deletion_dialog(
    const std::function<ftxui::Element()> deleted_entry,
    const std::function<bool(ftxui::Event)> handler) {

  ftxui::ButtonOption button_option;
  button_option.transform = [](const ftxui::EntryState &s) {
    auto style = s.active ? ftxui::bold : ftxui::nothing;
    return ftxui::text(s.label) | style | ftxui::center;
  };

  auto yes_button = ftxui::Button(
      "[Y]es", [this] { screen_.PostEvent(ftxui::Event::Character('y')); },
      button_option);

  auto no_button = ftxui::Button(
      "[N]o", [this] { screen_.PostEvent(ftxui::Event::Character('n')); },
      button_option);
  auto button_container = ftxui::Container::Horizontal({yes_button, no_button});

  auto dialog_renderer = ftxui::Renderer(
      button_container, [yes_button, no_button, deleted_entry, this] {
        auto dialog_content =
            ftxui::vbox({deleted_entry(), ftxui::filler(), ftxui::separator(),
                         ftxui::hbox({
                             ftxui::filler(),
                             yes_button->Render(),
                             ftxui::separatorEmpty() |
                                 ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                                             screen_.dimx() / 3),
                             no_button->Render(),
                             ftxui::filler(),
                         })});

        return ftxui::window(ftxui::text("Permanently delete selected file?"),
                             dialog_content) |
               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, screen_.dimx() / 3 * 2) |
               ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, screen_.dimy() / 3 * 2);
      });

  auto dialog_with_handler =
      dialog_renderer | ftxui::CatchEvent(handler) | ftxui::center;

  modal_ =
      ftxui::Modal(main_layout_, dialog_with_handler, &show_delete_dialog_);
}

void Ui::move_selected_up(const int max) {
  if (selected_ > 0) {
    selected_--;
  } else {
    selected_ = max;
  }
}
void Ui::move_selected_down(const int max) {
  if (selected_ < max) {
    selected_++;
  } else {
    selected_ = 0;
  }
}

void Ui::toggle_delete_dialog() { show_delete_dialog_ = !show_delete_dialog_; }

void Ui::enter_direcotry(
    const std::vector<fs::directory_entry> &curdir_entries) {
  update_curdir_string_entires(curdir_entries);
  selected_ = previous_selected_;
}

void Ui::leave_direcotry(const std::vector<fs::directory_entry> &curdir_entries,
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

void Ui::update_curdir_string_entires(
    const std::vector<fs::directory_entry> &curdir_entries) {
  curdir_string_entries_ =
      curdir_entries |
      std::views::transform([this](const fs::directory_entry &entry) {
        return this->format_directory_entries(entry);
      }) |
      std::ranges::to<std::vector>();
}

const std::string
Ui::format_directory_entries(const fs::directory_entry &entry) {
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

void Ui::render() { screen_.Loop(modal_); }

void Ui::exit() { screen_.Exit(); }

int Ui::selected() { return selected_; }

ftxui::Component &Ui::menu() { return menu_; }

ftxui::ScreenInteractive &Ui::screen() { return screen_; };

} // namespace duck
