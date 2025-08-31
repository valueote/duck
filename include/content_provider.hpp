#pragma once
#include "file_manager.hpp"
#include <filesystem>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <functional>
#include <stdexec/execution.hpp>
#include <string>
#include <vector>

namespace duck {
namespace fs = std::filesystem;

class ContentProvider {
private:
  FileManager &file_manager_;
  ftxui::Element deleted_entries(const int &selected);
  static ftxui::Element
  visible_entries(const std::vector<ftxui::Element> &all_entries,
                  const int &selected);
  ftxui::Element left_pane(const std::vector<ftxui::Element> &all_entries,
                           const int &selected);
  ftxui::Element right_pane(const ftxui::Element &entries_preview,
                            const std::string &text_preview,
                            const int &selected);

public:
  ContentProvider(FileManager &file_manager);

  ftxui::Component deletion_dialog(const int &selected,
                                   std::function<void()> yes,
                                   std::function<void()> no);
  ftxui::Component rename_dialog(int &cursor_position,
                                 std::string &rename_input);
  ftxui::Component creation_dialog(int &cursor_position,
                                   std::string &new_entry_input);
  ftxui::Component notification(std::string &content);
  ftxui::Component layout(const std::vector<ftxui::Element> &all_entries,
                          const ftxui::Element &entries_preview,
                          const std::string &text_preview, const int &selected);
};
} // namespace duck
