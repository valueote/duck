#pragma once
#include "filemanager.h"
#include "ui.h"
#include <ftxui/component/event.hpp>

namespace duck {
class InputHandler {
private:
  FileManager &file_manager_;
  UI &ui_;

  void open_file();

public:
  InputHandler(FileManager &file_manager, UI &ui);
  std::function<bool(ftxui::Event)> navigation_handler();
  std::function<bool(ftxui::Event)> deletetion_handler();
};

} // namespace duck
