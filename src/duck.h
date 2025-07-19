#pragma once
#include "filemanager.h"
#include "inputhandler.h"
#include "ui.h"

namespace duck {
class Duck {
private:
  FileManager file_manager_;
  UI ui_;
  InputHander input_handler;

  void setup_ui();

public:
  Duck();
  void run();
  void stop();
};

} // namespace duck
