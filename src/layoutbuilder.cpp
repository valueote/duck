#include "layoutbuilder.h"
#include "ui.h"
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>

namespace duck {
UiBuilder::UiBuilder(FileManager &file_manager, Ui &ui)
    : file_manager_{file_manager}, ui_{ui} {}

std::string UiBuilder::get_text_preview(const std::optional<fs::path> &path,
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
UiBuilder::get_directory_preview(const std::optional<fs::path> &dir_path) {
  if (!dir_path.has_value()) {
    return ftxui::text("Nothing to preview");
  }
  if (!fs::is_directory(dir_path.value())) {
    return ftxui::text("[ERROR]: Call get_directory_preview on the file");
  }

  file_manager_.update_preview_entries(ui_.selected());

  std::vector<ftxui::Element> lines;
  lines = file_manager_.preview_entries() |
          std::views::transform([this](const fs::directory_entry &entry) {
            return ftxui::text(ui_.format_directory_entries(entry));
          }) |
          std::ranges::to<std::vector>();

  if (lines.empty()) {
    lines.push_back(ftxui::text("[Empty folder]"));
  }

  return ftxui::vbox(std::move(lines));
}

std::function<ftxui::Element()> UiBuilder::layout() {
  return [this]() {
    auto left_pane =
        window(ftxui::text(" " + file_manager_.current_path().string() + " ") |
                   ftxui::bold,
               ui_.menu()->Render() | ftxui::vscroll_indicator | ftxui::frame |
                   ftxui::flex) |
        ftxui::flex;

    auto right_pane =
        window(ftxui::text(" Coneten Preview ") | ftxui::bold,
               [this] {
                 const auto selected_path_opt =
                     file_manager_.get_selected_entry(ui_.selected());
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
  };
}

std::function<ftxui::Element()> UiBuilder::deletion_dialog() {
  return [this]() { return ftxui::emptyElement(); };
}

} // namespace duck
