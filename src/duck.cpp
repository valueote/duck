#include "duck.hpp"
#include "app.hpp"
#include "event_bus.hpp"
#include "input_handler.hpp"
#include "ui.hpp"

namespace duck {
Duck::Duck()
    : input_handler_{event_bus_}, ui_{input_handler_}, app_{event_bus_, ui_} {}

void Duck::run() { app_.run(); }

} // namespace duck
