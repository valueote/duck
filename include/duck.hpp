#pragma once
#include "app.hpp"
#include "event_bus.hpp"
#include "ui.hpp"

namespace duck {
class Duck {
private:
  EventBus event_bus_;
  Ui ui_;
  App app_;

public:
  Duck();
  void run();
};

} // namespace duck