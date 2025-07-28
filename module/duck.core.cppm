module;

export module duck.core;

import duck.colorscheme;
import duck.filemanager;
import duck.contentprovider;
import duck.inputhandler;
import duck.ui;

namespace duck {
export class Duck {
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
