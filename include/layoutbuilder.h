#pragma once
#include "filemanager.h"
#include "ui.h"
#include <ftxui/dom/node.hpp>
namespace duck {
class LayoutBuilder {
private:
  UI &ui_;
  FileManager &file_manager_;

public:
  LayoutBuilder(FileManager &file_manager, UI &ui);
  std::string get_text_preview(const std::optional<fs::path> &path,
                               size_t max_lines = 100, size_t max_width = 80);

  ftxui::Element get_directory_preview(const std::optional<fs::path> &dir_path);
  ftxui::Element operator()();
};
} // namespace duck
