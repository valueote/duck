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

  ftxui::Element deleted_entries();

public:
  ContentProvider(Ui &ui, const ColorScheme &color_scheme);

  std::function<ftxui::Element(const ftxui::EntryState &state)>
  entries_transform();
  ftxui::Component deletion_dialog();
  std::function<ftxui::Element()> preview();
};
} // namespace duck
