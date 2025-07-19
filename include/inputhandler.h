#pragma once
#include "filemanager.h"
#include "ui.h"

namespace duck {
class InputHandler {
private:
  FileManager &file_manager_;
  UI &ui_;

public:
  InputHandler(FileManager &file_manager, UI &ui);
  void open_file();
  bool operator()(ftxui::Event event);
};

} // namespace duck
