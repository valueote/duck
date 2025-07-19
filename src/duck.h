#pragma once
#include "filemanager.h"
#include "inputhandler.h"
#include "layoutbuilder.h"
#include "ui.h"

namespace duck {
class Duck {
private:
  FileManager file_manager_;
  UI ui_;
  InputHander input_handler_;
  LayoutBuilder layout_builder_;

  void setup_ui();

public:
  Duck();
  void run();
  void stop();
};

} // namespace duck
