#pragma once
#include "content_provider.hpp"
#include "input_handler.hpp"
#include "ui.hpp"

namespace duck {
class Duck {
private:
  Ui ui_;
  InputHandler input_handler_;
  ContentProvider content_provider_;

  void setup_ui();

public:
  Duck();
  void run();
};

} // namespace duck
