#pragma once
#include "content_provider.h"
#include "input_handler.h"
#include "ui.h"

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
