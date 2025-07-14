#include "fs_utils.h"
#include <filesystem>
#include <unistd.h>
namespace duck {
fs_utils::fs_utils() : current_path_(fs::current_path()) { update_entries(); }

void fs_utils::update_entries() {
  for (const auto &entry : fs::directory_iterator(current_path_))
    entries_.push_back(entry);
}

std::vector<fs::directory_entry> fs_utils::get_direcotry_entries() {
  return entries_;
}

fs::path fs_utils::get_current_path() { return current_path_; }
} // namespace duck
