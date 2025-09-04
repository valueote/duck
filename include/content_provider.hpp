#pragma once
#include "app_event.hpp"
#include "app_state.hpp"
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
  ftxui::Element deleted_entries(const AppState &state);
  static ftxui::Element
  visible_entries(const std::vector<ftxui::Element> &all_entries,
                  const int &index);
  ftxui::Element left_pane(const MenuInfo &info);
  ftxui::Element right_pane(const EntryPreview &preview);

public:
  ContentProvider() = default;

  ftxui::Component deletion_dialog(const ftxui::Element &deleted_entries,

                                   std::function<void()> yes,
                                   std::function<void()> no);
  ftxui::Component rename_dialog(int &cursor_position,
                                 std::string &rename_input);
  ftxui::Component creation_dialog(int &cursor_position,
                                   std::string &new_entry_input);
  ftxui::Component notification(std::string &content);
  ftxui::Component layout(const MenuInfo &info, const EntryPreview &preview);
};
} // namespace duck
