#pragma once
#include "ui.h"
#include <ftxui/component/event.hpp>

namespace duck {
class InputHandler {
private:
  Ui &ui_;

  void open_file();

public:
  InputHandler(Ui &ui);
  std::function<bool(ftxui::Event)> navigation_handler();
  std::function<bool(ftxui::Event)> test_handler();
  std::function<bool(ftxui::Event)> deletetion_dialog_handler();
};

} // namespace duck
