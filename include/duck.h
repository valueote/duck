#pragma once
#include "colorscheme.h"
#include "content_provider.h"
#include "file_manager.h"
#include "input_handler.h"
#include "ui.h"

namespace duck {
class Duck {
private:
  FileManager file_manager_;
  ColorScheme color_scheme_;
  Ui ui_;
  InputHandler input_handler_;
  ContentProvider content_provider_;

  void setup_ui();

public:
  Duck();
  void run();
  void stop();
};

} // namespace duck
