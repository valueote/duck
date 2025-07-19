#include "filemanager.h"
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <optional>
#include <print>
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
  if (!fs::is_directory(path)) {
    return;
  }

  entries.clear();

  try {
    std::vector<fs::directory_entry> dirs;
    std::vector<fs::directory_entry> files;

    dirs.reserve(128);
    files.reserve(128);

    for (const auto &entry : fs::directory_iterator(path)) {
      (entry.is_directory() ? dirs : files).push_back(entry);
    }

    entries.reserve(dirs.size() + files.size());
    std::ranges::sort(dirs);
    std::ranges::sort(files);
    std::ranges::copy(dirs, std::back_inserter(entries));
    std::ranges::copy(files, std::back_inserter(entries));

  } catch (const std::exception &e) {
    std::print(stderr, "[ERROR]: {} in load_directory_entries", e.what());
  }
}

void FileManager::update_curdir_entries() {
  load_directory_entries(current_path_, curdir_entries_);
}

void FileManager::update_preview_entries(const int &selected) {
  load_directory_entries(curdir_entries_[selected].path(), preview_entries_);
}

const fs::path &FileManager::current_path() const { return current_path_; }

const fs::path &FileManager::cur_parent_path() const { return parent_path_; }

const fs::path &FileManager::previous_path() const { return previous_path_; }

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
  update_curdir_entries();
}

const std::optional<fs::directory_entry>
FileManager::get_selected_entry(const int &selected) const {
  if (curdir_entries_.empty()) {
    return std::nullopt;
  }
  return curdir_entries_[selected];
}

bool FileManager::delete_selected_entry(const int selected) {
  return fs::remove(curdir_entries_[selected]);
}

} // namespace duck
