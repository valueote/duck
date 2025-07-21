#pragma once
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
class Ui {
private:
  std::vector<std::string> curdir_string_entries_;
  std::vector<std::string> previewdir_string_entries_;

  ftxui::ScreenInteractive screen_;
  ftxui::Component layout_;
  ftxui::MenuOption menu_option_;
  ftxui::Component modal_;
  ftxui::Component menu_;

  std::string file_preview_content_;
  std::vector<std::string> dir_preview_content_;

  int selected_;
  int previous_selected_;
  bool show_delete_dialog_;

public:
  Ui();

  void set_layout(const std::function<ftxui::Element()> layout);
  void set_input_handler(const std::function<bool(ftxui::Event)> handler);
  void set_deletion_dialog(const std::function<ftxui::Element()> dialog,
                           const std::function<bool(ftxui::Event)> handler);
  void toggle_delete_dialog();
  void enter_direcotry(const std::vector<fs::directory_entry> &curdir_entries);
  void leave_direcotry(const std::vector<fs::directory_entry> &curdir_entries,
                       const fs::path &previous_path);
  void update_curdir_string_entires(
      const std::vector<fs::directory_entry> &curdir_entries);
  const std::string format_directory_entries(const fs::directory_entry &entry);

  void render();
  void exit();
  int selected();
  ftxui::Component &menu();
  ftxui::ScreenInteractive &screen();
};

} // namespace duck
