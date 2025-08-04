#include "duck.h"
#include <boost/asio.hpp>
int main() {
  duck::Duck d;
  d.run();
}

// TODO: Improve async to implement better performance
// TODO: Add lazy loading for directory entries
// TODO: implement error handler for filemanager yanking and cutting
// TODO: Implement better log
// TODO: Forbid delete current dir when navigating to it
// TODO: Add preview for image
// TODO: implement better color scheme
// TODO: Add realtime response for filesystem changing;
// TODO: Add code hilighting for preview
// TODO: Add a parent dir pane
