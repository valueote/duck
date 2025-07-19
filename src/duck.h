#pragma once
#include "filemanager.h"
#include "ui.h"

namespace duck {
class Duck {
private:
  FileManager file_manager_;
  UI ui_;

public:
  Duck();
  void run();
  void stop();
};

} // namespace duck
