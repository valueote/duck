#pragma once
#include "filemanager.h"
#include "inputhandler.h"
#include "layoutbuilder.h"
#include "ui.h"

namespace duck {
class Duck {
private:
  FileManager file_manager_;
  Ui ui_;
  InputHandler input_handler_;
  UiBuilder ui_builder_;

  void setup_ui();

public:
  Duck();
  void run();
  void stop();
};

} // namespace duck
