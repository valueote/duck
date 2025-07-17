#include "duck.h"

namespace duck {
Duck::Duck() : ui_(filemanager_) {}

void Duck::run() { ui_.render(); }

} // namespace duck
