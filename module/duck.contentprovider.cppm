module;

#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/node.hpp>

export module duck.contentprovider;

import duck.ui;
import duck.filemanager;
import duck.colorscheme;

namespace duck {
export class ContentProvider {
private:
  Ui &ui_;
  FileManager &file_manager_;
  const ColorScheme &color_scheme_;

  std::string get_text_preview(const std::optional<fs::path> &path,
                               size_t max_lines = 100, size_t max_width = 80);

  std::string bat_text_preview(const std::optional<fs::path> &path,
                               size_t max_lines = 100, size_t max_width = 80);
  ftxui::Element get_directory_preview(const std::optional<fs::path> &dir_path);
  ftxui::Element deleted_entries();

public:
  ContentProvider(FileManager &file_manager, Ui &ui,
                  const ColorScheme &color_scheme);

  std::function<ftxui::Element(const ftxui::EntryState &state)>
  entries_transform();
  std::function<ftxui::Element()> preview();
  ftxui::Component deletion_dialog();
};
} // namespace duck
