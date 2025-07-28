module;
#include <ftxui/component/event.hpp>
#include <functional>

export module duck.inputhandler;

import duck.ui;
import duck.filemanager;

namespace duck {
export class InputHandler {
private:
  FileManager &file_manager_;
  Ui &ui_;

  void open_file();

public:
  InputHandler(FileManager &file_manager, Ui &ui);
  std::function<bool(ftxui::Event)> navigation_handler();
  std::function<bool(ftxui::Event)> deletetion_dialog_handler();
};

} // namespace duck
