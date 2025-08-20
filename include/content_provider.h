#pragma once
#include "colorscheme.h"
#include "stdexec/__detail/__execution_fwd.hpp"
#include "ui.h"
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <functional>
#include <stdexec/execution.hpp>

namespace duck {
class ContentProvider {
private:
  Ui &ui_;
  const ColorScheme &color_scheme_;
  std::mutex mutex_;
  ftxui::Element left_pane(int width);
  ftxui::Element right_pane(int width);
  ftxui::Element deleted_entries();
  ftxui::Element left_pane();
  ftxui::Element right_pane();

public:
  ContentProvider(Ui &ui, const ColorScheme &color_scheme);

  std::function<ftxui::Element(const ftxui::EntryState &state)>
  menu_entries_transform();
  ftxui::Component deletion_dialog();
  ftxui::Component rename_dialog();
  ftxui::Component layout();
};
} // namespace duck
