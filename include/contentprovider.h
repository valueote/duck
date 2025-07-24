#pragma once
#include "filemanager.h"
#include "ui.h"
#include <ftxui/dom/node.hpp>
#include <functional>
namespace duck {
class ContentProvider {
private:
  Ui &ui_;
  FileManager &file_manager_;

  std::string get_text_preview(const std::optional<fs::path> &path,
                               size_t max_lines = 100, size_t max_width = 80);

  std::string bat_text_preview(const std::optional<fs::path> &path,
                               size_t max_lines = 100, size_t max_width = 80);
  ftxui::Element get_directory_preview(const std::optional<fs::path> &dir_path);

public:
  ContentProvider(FileManager &file_manager, Ui &ui);
  std::function<ftxui::Element()> preview();
  std::function<ftxui::Element()> deleted_entries();
};
} // namespace duck
