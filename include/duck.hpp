#pragma once
#include "content_provider.hpp"
#include "file_manager.hpp"
#include "input_handler.hpp"
#include "ui.hpp"

namespace duck {
class Duck {
private:
  ContentProvider content_provider_;
  Ui ui_;
  FileManager file_manager_;
  InputHandler input_handler_;

  void setup_ui();

public:
  Duck();
  void run();
};

} // namespace duck
