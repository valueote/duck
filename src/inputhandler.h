#pragma once
#include "filemanager.h"
#include "ui.h"

namespace duck {
class InputHander {
private:
  FileManager &file_manager_;
  UI &ui_;

public:
  InputHander(FileManager &file_manager, UI &ui);
  void open_file();
  bool operator()(ftxui::Event event);
};

} // namespace duck
