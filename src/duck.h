#pragma once
#include "filemanager.h"
#include "ui.h"

namespace duck {
class Duck {
private:
  FileManager filemanager_;
  UI ui_;

public:
  Duck();
  void run();
  void stop();
};

} // namespace duck
