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
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
namespace duck {

UI::UI(FileManager &file_manager)
    : selected_{0}, previous_selected_(0),
      screen_{ftxui::ScreenInteractive::Fullscreen()},
      file_manager_{file_manager} {
  update_curdir_string_entires();
  build_menu();
  setup_layout();
}
UI::~UI() {}

// TODO: Add a parent dir plane
void UI::build_menu() {
  menu_option_.focused_entry = &selected_;
  menu_ = Menu(&curdir_string_entries_, &(selected_), menu_option_) |
          ftxui::CatchEvent([this](ftxui::Event event) {
            if (event == ftxui::Event::Return) {
              open_file();
              return true;
            }
            if (event == ftxui::Event::Character('l')) {
              if (file_manager_.get_selected_entry(selected_).has_value() &&
                  fs::is_directory(
                      file_manager_.get_selected_entry(selected_).value())) {
                move_down_direcotry();
              }
              return true;
            }
            if (event == ftxui::Event::Character('h')) {
              move_up_direcotry();
              return true;
            }
            if (event == ftxui::Event::Character('q')) {
              screen_.Exit();
              return true;
            }
            return false;
          });
}

void UI::setup_layout() {
  layout_ = ftxui::Renderer(menu_, [this] {
    auto left_pane =
        window(ftxui::text(" " + file_manager_.current_path().string() + " ") |
                   ftxui::bold,
               menu_->Render() | ftxui::vscroll_indicator | ftxui::frame |
                   ftxui::flex) |
        ftxui::flex;

    auto right_pane =
        window(ftxui::text(" Coneten Preview ") | ftxui::bold,
               [this] {
                 const auto selected_path_opt =
                     file_manager_.get_selected_entry(selected_);
                 if (!selected_path_opt.has_value()) {
                   return ftxui::text("No item selected");
                 }
                 if (fs::is_directory(selected_path_opt.value())) {
                   return get_directory_preview(selected_path_opt);
                 }
                 return ftxui::paragraph(get_text_preview(selected_path_opt));
               }() |
                   ftxui::vscroll_indicator | ftxui::frame | ftxui::flex) |
        ftxui::flex;

    return hbox(left_pane, ftxui::separator(), right_pane);
  });
}

std::string UI::get_text_preview(const std::optional<fs::path> &path,
                                 size_t max_lines, size_t max_width) {
  if (!path.has_value()) {
    return "No file to preview";
  }

  if (fs::is_directory(path.value()))
    return "[ERROR]: Call get_text_preview on the directory";

  std::ifstream file(path.value());

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

ftxui::Element
UI::get_directory_preview(const std::optional<fs::path> &dir_path) {
  if (!dir_path.has_value()) {
    return ftxui::text("Nothing to preview");
  }
  if (!fs::is_directory(dir_path.value())) {
    return ftxui::text("[ERROR]: Call get_directory_preview on the file");
  }

  file_manager_.update_preview_entries(selected_);

  std::vector<ftxui::Element> lines;
  lines = file_manager_.preview_entries() |
          std::views::transform([this](const fs::directory_entry &entry) {
            return ftxui::text(this->format_directory_entries(entry));
          }) |
          std::ranges::to<std::vector>();

  if (lines.empty()) {
    lines.push_back(ftxui::text("[Empty folder]"));
  }

  return ftxui::vbox(std::move(lines));
}

void UI::move_down_direcotry() {
  file_manager_.update_current_path(fs::canonical(
      file_manager_.get_selected_entry(selected_).value().path()));
  file_manager_.update_curdir_entries();
  update_curdir_string_entires();
  selected_ = previous_selected_;
}

void UI::move_up_direcotry() {
  file_manager_.update_current_path(file_manager_.cur_parent_path());
  file_manager_.update_curdir_entries();
  update_curdir_string_entires();
  previous_selected_ = selected_;
  set_selected_previous_dir();
}

void UI::set_selected_previous_dir() {
  const auto &entries = file_manager_.curdir_entries();
  const auto &previous = file_manager_.previous_path();

  if (auto it = std::ranges::find(entries, previous); it != entries.end()) {
    selected_ = static_cast<int>(std::distance(entries.begin(), it));
  }
}

void UI::update_curdir_string_entires() {
  curdir_string_entries_ =
      file_manager_.curdir_entries() |
      std::views::transform([this](const fs::directory_entry &entry) {
        return this->format_directory_entries(entry);
      }) |
      std::ranges::to<std::vector>();
}
void UI::render() { screen_.Loop(layout_); }

// FIX: key conficting when running new proc
void UI::open_file() {
  const static std::unordered_map<std::string, std::string> handlers = {
      {".txt", "nvim"}, {".cpp", "nvim"}, {"none", "none"}};
  auto selected_file_opt = file_manager_.get_selected_entry(selected_);
  if (!selected_file_opt.has_value()) {
    return;
  }

  std::string ext = selected_file_opt.value().path().extension().string();
  if (!handlers.contains(ext)) {
    return;
  }

  screen_.WithRestoredIO([&] {
    pid_t pid = fork();
    if (pid == -1) {
      std::print(stderr, "[ERROR]: fork fail in open_file");
      return;
    }

    if (pid == 0) {
      const char *handler = handlers.at(ext).c_str();
      const char *file_path = selected_file_opt->path().c_str();

      execlp(handler, handler, file_path, nullptr);

      std::exit(EXIT_FAILURE);
    } else {
      int status;
      waitpid(pid, &status, 0);
    }
  })();
}

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
