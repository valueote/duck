#pragma once
#include "filemanager.h"
#include <filesystem>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <optional>
#include <string>
#include <vector>

namespace duck {
namespace fs = std::filesystem;
class UI {
private:
  FileManager &file_manager_;

  std::vector<std::string> curdir_string_entries_;
  std::vector<std::string> previewdir_string_entries_;

  ftxui::ScreenInteractive screen_;
  ftxui::Component layout_;
  ftxui::MenuOption menu_option_;
  ftxui::Component menu_;

  std::string file_preview_content_;
  std::vector<std::string> dir_preview_content_;

  int selected_;
  int previous_selected_;

  void build_menu();
  void setup_layout();
  std::string get_text_preview(const std::optional<fs::path> &path,
                               size_t max_lines = 50, size_t max_width = 80);
  ftxui::Element get_directory_preview(const std::optional<fs::path> &dir_path);
  void move_down_direcotry();
  void move_up_direcotry();
  void set_selected_previous_dir();
  void update_curdir_string_entires();

public:
  UI(FileManager &file_manager);
  ~UI();

  std::string format_directory_entries(const fs::directory_entry &entry);
  void render();
};

} // namespace duck
