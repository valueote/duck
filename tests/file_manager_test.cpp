#include "doctest.h"
#include "file_manager.h"
#include <filesystem>

namespace fs = std::filesystem;

TEST_CASE("file_manager toggle_hidden_entries_test") {
  fs::path home{"/home/vivy/"};
  duck::FileManager::update_current_path(home);
  duck::FileManager::update_curdir_entries(false);
  auto entries = duck::FileManager::curdir_entries();
  duck::FileManager::toggle_hidden_entries();
  duck::FileManager::update_curdir_entries(false);
  auto new_entries = duck::FileManager::curdir_entries();
  CHECK(entries.size() != new_entries.size());
}
