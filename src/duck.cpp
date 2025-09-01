#include "duck.hpp"
#include "app.hpp"
#include "event_bus.hpp"
#include "ui.hpp"

namespace duck {
Duck::Duck() : ui_{}, app_{event_bus_, ui_} {}

void Duck::run() { app_.run(); }

} // namespace duck