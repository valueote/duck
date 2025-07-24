#include "filemanager.h"
#include <algorithm>
#include <filesystem>
#include <format>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <unordered_map>
namespace duck {
FileManager::FileManager()
    : current_path_{fs::current_path()},
      parent_path_{current_path_.parent_path()}, is_yanking_{false},
      is_cutting_{false}, show_hidden_{false} {
  update_curdir_entries();
  if (fs::is_directory(curdir_entries_[0])) {
    update_preview_entries(0);
  }
  marked_entires_.reserve(64);
  clipboard_entries_.reserve(64);
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

const std::vector<fs::directory_entry> &FileManager::marked_entries() const {
  return marked_entires_;
}

std::vector<std::string> FileManager::curdir_entries_string() const {
  return curdir_entries_ |
         std::views::transform([this](const fs::directory_entry &entry) {
           return format_directory_entries(entry);
         }) |
         std::ranges::to<std::vector>();
}

std::vector<std::string> FileManager::preview_entries_string() const {
  return {};
}
std::vector<std::string> FileManager::marked_entries_string() const {
  return {};
}

int FileManager::get_previous_path_index() const {
  if (auto it = std::ranges::find(curdir_entries_, previous_path_);
      it != curdir_entries_.end()) {
    return static_cast<int>(std::distance(curdir_entries_.begin(), it));
  }
  return 0;
}

bool FileManager::yanking() const { return is_yanking_; }
bool FileManager::cutting() const { return is_cutting_; }

std::optional<fs::directory_entry>
FileManager::get_selected_entry(const int &selected) const {
  if (curdir_entries_.empty()) {
    return std::nullopt;
  } else if (selected < 0 || selected >= curdir_entries_.size()) {
    return std::nullopt;
  }
  return curdir_entries_[selected];
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
      if (entry.path().filename().string().starts_with('.') && !show_hidden_) {
        continue;
      }
      (entry.is_directory() ? dirs : files).push_back(entry);
    }

    entries.reserve(dirs.size() + files.size());
    std::ranges::sort(dirs);
    std::ranges::sort(files);
    std::ranges::copy(dirs, std::back_inserter(entries));
    std::ranges::copy(files, std::back_inserter(entries));

  } catch (const std::exception &e) {
    std::println(stderr, "[ERROR]: {} in load_directory_entries", e.what());
  }
}

void FileManager::update_curdir_entries() {
  load_directory_entries(current_path_, curdir_entries_);
}

void FileManager::update_preview_entries(const int &selected) {
  load_directory_entries(curdir_entries_[selected].path(), preview_entries_);
}

void FileManager::update_current_path(const fs::path &new_path) {
  previous_path_ = current_path_;
  current_path_ = new_path;
  parent_path_ = current_path_.parent_path();
  update_curdir_entries();
}

void FileManager::toggle_mark_on_selected(const int &selected) {
  if (curdir_entries_.empty() || selected < 0 ||
      selected >= curdir_entries_.size()) {
    return;
  }

  if (auto it = std::ranges::find(marked_entires_, curdir_entries_[selected]);
      it != marked_entires_.end()) {
    marked_entires_.erase(it);
  } else {
    marked_entires_.push_back(curdir_entries_[selected]);
  }
}

void FileManager::toggle_hidden_entries() { show_hidden_ = !show_hidden_; }

void FileManager::start_yanking() {
  is_yanking_ = true;
  is_cutting_ = false;
}

void FileManager::start_cutting() {
  is_cutting_ = true;
  is_yanking_ = false;
}

void FileManager::paste(const int &selected) {
  if (!is_cutting_ && !is_yanking_) {
    std::println(stderr, "[ERROR] invalid state when pasting");
    return;
  }

  if (marked_entires_.empty()) {
    clipboard_entries_.push_back(curdir_entries_[selected]);
  } else {
    clipboard_entries_ = std::move(marked_entires_);
  }
  marked_entires_.clear();

  try {
    for (const auto &entry : clipboard_entries_) {
      const fs::path dest_path = current_path_ / entry.path().filename();
      if (is_yanking_) {
        fs::copy(entry.path(), dest_path, fs::copy_options::recursive);
      } else if (is_cutting_) {
        fs::rename(entry.path(), dest_path);
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::println(stderr, "[ERORR] {}", e.what());
  }
}
bool FileManager::is_marked(const fs::directory_entry &entry) const {
  return std::ranges::find(marked_entires_, entry) != marked_entires_.end();
}

bool FileManager::delete_selected_entry(const int selected) {
  return delete_entry(curdir_entries_[selected]);
}

bool FileManager::delete_marked_entries() {
  if (marked_entires_.empty()) {
    std::println(stderr, "[ERROR] try to delete empty file");
    return false;
  }

  for (auto &entry : marked_entires_) {
    delete_entry(entry);
  }

  marked_entires_.clear();
  return true;
}

void FileManager::clear_marked_entries() { marked_entires_.clear(); }

bool FileManager::delete_entry(fs::directory_entry &entry) {
  if (!fs::exists(entry)) {
    std::println(stderr, "[ERROR] try to delete an unexisted file");
    return false;
  }
  if (fs::is_directory(entry)) {
    return fs::remove_all(entry);
  } else {
    return fs::remove(entry);
  }
  return true;
}

std::string
FileManager::entry_name_with_icon(const fs::directory_entry &entry) const {
  static const std::unordered_map<std::string, std::string> extension_icons{
      {".txt", "\uf15c"}, {".md", "\ueeab"},   {".cpp", "\ue61d"},
      {".hpp", "\uf0fd"}, {".h", "\uf0fd"},    {".c", "\ue61e"},
      {".jpg", "\uf4e5"}, {".jpeg", "\uf4e5"}, {".png", "\uf4e5"},
      {".gif", "\ue60d"}, {".pdf", "\ue67d"},  {".zip", "\ue6aa"},
      {".mp3", "\uf001"}, {".mp4", "\uf03d"},  {".json", "\ue60b"},
      {".log", "\uf4ed"}, {".csv", "\ueefc"},
  };

  const auto filename = entry.path().filename().string();
  if (fs::is_directory(entry)) {
    return std::format("\uf4d3 {}", filename);
  }

  auto ext = entry.path().extension().string();
  std::ranges::transform(ext, ext.begin(), [](char c) {
    return static_cast<char>(std::tolower(c));
  });

  auto icon_it = extension_icons.find(ext);
  const std::string &icon =
      icon_it != extension_icons.end() ? icon_it->second : "\uf15c";

  return std::format("{} {}", icon, filename);
}

std::string
FileManager::format_directory_entries(const fs::directory_entry &entry) const {
  const std::string selected_marker = is_marked(entry) ? "█ " : "  ";

  return selected_marker + entry_name_with_icon(entry);
}

} // namespace duck
