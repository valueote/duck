#include "filemanager.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
namespace duck {
FileManager::FileManager()
    : current_path_(fs::current_path()),
      parent_path_(current_path_.parent_path()) {
  update_curdir_entries();
  if (fs::is_directory(curdir_entries_[0])) {
    update_preview_entries(0);
  }
}

void FileManager::load_directory_entries(
    const fs::path &path, std::vector<fs::directory_entry> &entries) {
  if (!fs::exists(path) || !fs::is_directory(path)) {
    return;
  }
  entries.clear();
  try {
    std::vector<fs::directory_entry> dirs;
    std::vector<fs::directory_entry> files;

    for (const auto &entry : fs::directory_iterator(path)) {
      if (entry.is_directory()) {
        dirs.push_back(entry);
      } else {
        files.push_back(entry);
      }
    }
    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());
    entries.insert(entries.end(), dirs.begin(), dirs.end());
    entries.insert(entries.end(), files.begin(), files.end());

  } catch (const std::exception &e) {
    std::cerr << e.what();
  }
}

void FileManager::update_curdir_entries() {
  load_directory_entries(current_path_, curdir_entries_);
}

void FileManager::update_preview_entries(int selected) {
  load_directory_entries(curdir_entries_[selected].path(), preview_entries_);
}

const fs::path &FileManager::current_path() const { return current_path_; }

const fs::path &FileManager::parent_path() const { return parent_path_; }

const std::vector<fs::directory_entry> &FileManager::curdir_entries() const {
  return curdir_entries_;
}
const std::vector<fs::directory_entry> &FileManager::preview_entries() const {
  return preview_entries_;
}

void FileManager::update_current_path(const fs::path &new_path) {
  previous_path_ = current_path_;
  current_path_ = new_path;
  parent_path_ = current_path_.parent_path();
}

std::optional<fs::directory_entry>
FileManager::get_selected_entry(int selected) {
  if (curdir_entries_.empty()) {
    return std::nullopt;
  }
  return curdir_entries_[selected];
}

} // namespace duck
