#pragma once
#include "app.hpp"
#include "event_bus.hpp"
#include "file_manager.hpp"
#include "input_handler.hpp"
#include "ui.hpp"

namespace duck {
class Duck {
private:
  EventBus event_bus_;
  InputHandler input_handler_;
  Ui ui_;
  AppState state_;
  FileManager file_manager_;
  App app_;

public:
  Duck();
  void run();
};

} // namespace duck
