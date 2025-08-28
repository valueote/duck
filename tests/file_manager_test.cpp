#include "doctest.h"
#include "file_manager.h"

TEST_CASE("file_manager toggle_hidden_test") {
  duck::FileManager::toggle_hidden_entries();
}
