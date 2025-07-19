#pragma once
#include "filemanager.h"
#include <filesystem>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <functional>
#include <string>
#include <vector>

namespace duck {
namespace fs = std::filesystem;
class UI {
private:
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

public:
  UI();

  void setup_layout(std::function<ftxui::Element()> layout_builder);
  void move_down_direcotry(FileManager &file_manager_);
  void move_up_direcotry(FileManager &file_manager_);
  void set_selected_previous_dir(FileManager &file_manager);
  void update_curdir_string_entires(FileManager &file_manager);
  void exit();
  int get_selected();
  ftxui::Component &get_menu();
  ftxui::ScreenInteractive &get_screen();

  void set_input_handler(std::function<bool(ftxui::Event)> handler);
  std::string format_directory_entries(const fs::directory_entry &entry);

  void render();
};

} // namespace duck
