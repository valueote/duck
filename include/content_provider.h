#pragma once
#include "ui.h"
#include <filesystem>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <functional>
#include <stdexec/execution.hpp>

namespace duck {
namespace fs = std::filesystem;

class ContentProvider {
private:
  Ui &ui_;
  ftxui::Element deleted_entries();
  ftxui::Element visible_entries();
  ftxui::Element left_pane();
  ftxui::Element right_pane();

public:
  ContentProvider(Ui &ui);

  std::function<ftxui::Element(const ftxui::EntryState &state)>
  menu_entries_transform();
  ftxui::Component deletion_dialog();
  ftxui::Component rename_dialog();
  ftxui::Component creation_dialog();
  ftxui::Component notification();
  ftxui::Component layout();
  static std::vector<ftxui::Element>
  entries_to_elements(const std::vector<fs::directory_entry> &entries);
};
} // namespace duck
