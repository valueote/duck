#include "duck.h"
#include <chrono>
#include <exec/linux/io_uring_context.hpp>
#include <print>

int main() {
  duck::Duck d;
  d.run();
}

// TODO: Forbid delete current dir when navigating to it
// TODO: implement task cancelation
// TODO: Add lazy loading for directory entries
// TODO: Implement better log
// TODO: Add preview for image
// TODO: implement better color scheme
// TODO: Add realtime response for filesystem changing;
// TODO: Add code hilighting for preview
// TODO: Add a parent dir pane
