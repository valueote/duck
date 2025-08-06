#include "file_manager.h"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <mutex>
#include <optional>
#include <print>
#include <ranges>
#include <shared_mutex>
#include <stdexec/execution.hpp>
#include <unordered_map>
#include <utility>
#include <vector>
namespace duck {

constexpr size_t lru_cache_size = 50;

FileManager::FileManager()
    : current_path_{fs::current_path()},
      parent_path_{current_path_.parent_path()}, lru_cache_{lru_cache_size},
      is_yanking_{false}, is_cutting_{false}, show_hidden_{false} {

  load_directory_entries_without_lock(current_path_, false);

  if (fs::is_directory(curdir_entries_[0])) {
    update_preview_entries_without_lock(0);
  }

  marked_entires_.reserve(64);
  clipboard_entries_.reserve(64);
}

FileManager &FileManager::instance() {
  static FileManager instance;
  return instance;
}

const fs::path &FileManager::current_path() {
  std::shared_lock lock{file_mutex_};
  return instance().current_path_;
}

const fs::path &FileManager::cur_parent_path() {
  std::shared_lock lock{file_mutex_};
  return instance().parent_path_;
}

const fs::path &FileManager::previous_path() {
  std::shared_lock lock{file_mutex_};
  return instance().previous_path_;
}

const std::vector<fs::directory_entry> &FileManager::curdir_entries() {
  std::shared_lock lock{file_mutex_};
  return instance().curdir_entries_;
}

const std::vector<fs::directory_entry> &FileManager::preview_entries() {
  std::shared_lock lock{file_mutex_};
  return instance().preview_entries_;
}

const std::vector<fs::directory_entry> &FileManager::marked_entries() {
  std::shared_lock lock{file_mutex_};
  return instance().marked_entires_;
}

std::vector<std::string> FileManager::curdir_entries_string() {
  auto &instance = FileManager::instance();
  if (instance.curdir_entries_.empty()) {
    return {"[Empty folder]"};
  }
  return instance.curdir_entries_ |
         std::views::transform([&instance](const fs::directory_entry &entry) {
           std::shared_lock lock{FileManager::file_mutex_};
           return instance.format_directory_entries_without_lock(entry);
         }) |
         std::ranges::to<std::vector>();
}

int FileManager::previous_path_index() {
  if (auto it = std::ranges::find(instance().curdir_entries_,
                                  instance().previous_path_);
      it != instance().curdir_entries_.end()) {
    return static_cast<int>(
        std::distance(instance().curdir_entries_.begin(), it));
  }
  return 0;
}

bool FileManager::yanking() {
  std::shared_lock lock{file_mutex_};
  return instance().is_yanking_;
}

bool FileManager::cutting() {
  std::shared_lock lock{file_mutex_};
  return instance().is_cutting_;
}

std::expected<fs::directory_entry, std::string>
FileManager::selected_entry(const int &selected) {
  std::shared_lock lock{file_mutex_};
  auto &instance = FileManager::instance();

  if (instance.curdir_entries_.empty()) {
    return std::unexpected("No entries in current directory");
  }
  if (selected < 0 || selected >= instance.curdir_entries_.size()) {
    return std::unexpected("Selected index out of range: " +
                           std::to_string(selected));
  }
  return instance.curdir_entries_[selected];
}

std::vector<fs::directory_entry>
FileManager::load_directory_entries_without_lock(const fs::path &path,
                                                 bool preview) {
  if (!fs::is_directory(path)) {
    return {};
  }
  auto &entries = (preview ? preview_entries_ : curdir_entries_);

  auto cache = lru_cache_.get(path);
  if (cache.has_value()) {
    entries = std::move(cache.value());
    return entries;
  }

  entries.clear();
  std::vector<fs::directory_entry> dirs;
  std::vector<fs::directory_entry> files;
  dirs.reserve(512);
  files.reserve(512);

  entries.reserve(1024);
  for (auto &entry : fs::directory_iterator(
           path, fs::directory_options::skip_permission_denied)) {
    if (entry.path().empty()) {
      continue;
    }
    if (entry.path().filename().native()[0] == '.' && !show_hidden_) {
      continue;
    }
    (fs::is_directory(entry) ? dirs : files).push_back(std::move(entry));
  }

  std::ranges::sort(dirs);
  std::ranges::sort(files);
  std::ranges::move(dirs, std::back_inserter(entries));
  std::ranges::move(files, std::back_inserter(entries));

  lru_cache_.insert(path, entries);

  return entries;
}

std::generator<std::vector<fs::directory_entry>>
FileManager::lazy_load_directory_entries_without_lock(const fs::path &path,
                                                      bool preview,
                                                      const size_t &chunk) {

  if (!fs::is_directory(path)) {
    co_return;
  }

  auto &entries = (preview ? preview_entries_ : curdir_entries_);
  entries.clear();

  std::vector<fs::directory_entry> dirs;
  std::vector<fs::directory_entry> files;
  dirs.reserve(128);
  files.reserve(128);
  size_t loaded_size{0};
  for (const auto &entry : fs::directory_iterator(
           path, fs::directory_options::skip_permission_denied)) {
    if (entry.path().empty() || !fs::exists(entry)) {
      continue;
    }

    if (entry.path().filename().string().starts_with('.') && !show_hidden_) {
      continue;
    }

    (entry.is_directory() ? dirs : files).push_back(std::move(entry));
    if (++loaded_size >= chunk) {
      co_yield entries;
      loaded_size = 0;
    }
  }

  entries.reserve(dirs.size() + files.size());
  std::ranges::sort(dirs);
  std::ranges::sort(files);
  std::ranges::move(dirs, std::back_inserter(entries));
  std::ranges::move(files, std::back_inserter(entries));

  co_return;
}

void FileManager::update_curdir_entries() {
  std::unique_lock lock{file_mutex_};
  instance().load_directory_entries_without_lock(instance().current_path_,
                                                 false);
}

void FileManager::update_preview_entries(const int &selected) {
  std::unique_lock lock{file_mutex_};
  instance().update_preview_entries_without_lock(selected);
}

void FileManager::update_preview_entries_without_lock(const int &selected) {
  if (curdir_entries_.empty()) {
    return;
  }
  if (selected < 0 || selected >= curdir_entries_.size()) {
    return;
  }
  if (not fs::is_directory(curdir_entries_[selected])) {
    return;
  }
  load_directory_entries_without_lock(curdir_entries_[selected].path(), true);
}

void FileManager::toggle_mark_on_selected(const int &selected) {
  std::unique_lock lock{file_mutex_};
  auto &instance = FileManager::instance();
  if (instance.curdir_entries_.empty() || selected < 0 ||
      selected >= instance.curdir_entries_.size()) {
    return;
  }
  if (auto it = std::ranges::find(instance.marked_entires_,
                                  instance.curdir_entries_[selected]);
      it != instance.marked_entires_.end()) {
    instance.marked_entires_.erase(it);
  } else {
    instance.marked_entires_.push_back(instance.curdir_entries_[selected]);
  }
}

void FileManager::toggle_hidden_entries() {
  std::unique_lock lock{file_mutex_};
  instance().show_hidden_ = !instance().show_hidden_;
}

void FileManager::start_yanking() {
  std::unique_lock lock{file_mutex_};
  instance().is_yanking_ = true;
  instance().is_cutting_ = false;
}

void FileManager::start_cutting() {
  std::unique_lock lock{file_mutex_};
  instance().is_cutting_ = true;
  instance().is_yanking_ = false;
}

void FileManager::paste(const int &selected) {
  auto &instance = FileManager::instance();
  if (!instance.is_cutting_ && !instance.is_yanking_) {
    std::println(stderr, "[ERROR] invalid state when pasting");
    return;
  }

  std::unique_lock lock{file_mutex_};
  if (instance.marked_entires_.empty()) {
    instance.clipboard_entries_.push_back(instance.curdir_entries_[selected]);
  } else {
    instance.clipboard_entries_ = std::move(instance.marked_entires_);
  }
  instance.marked_entires_.clear();

  try {
    for (const auto &entry : instance.clipboard_entries_) {
      fs::path dest_path = instance.current_path_ / entry.path().filename();
      if (fs::exists(dest_path)) {
        dest_path =
            instance.current_path_ / (entry.path().filename().string() + "_1");
      }
      if (instance.is_yanking_) {
        fs::copy(entry.path(), dest_path, fs::copy_options::recursive);
      } else if (instance.is_cutting_) {
        fs::rename(entry.path(), dest_path);
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::println(stderr, "[ERORR] {}", e.what());
  }
}

bool FileManager::is_marked(const fs::directory_entry &entry) {
  std::shared_lock lock(file_mutex_);
  return std::ranges::find(instance().marked_entires_, entry) !=
         instance().marked_entires_.end();
}

bool FileManager::delete_selected_entry(const int selected) {
  std::unique_lock lock{file_mutex_};
  return instance().delete_entry_without_lock(
      instance().curdir_entries_[selected]);
}

bool FileManager::delete_marked_entries() {

  std::unique_lock lock(file_mutex_);
  auto &instance = FileManager::instance();
  if (instance.marked_entires_.empty()) {
    std::println(stderr, "[ERROR] try to delete empty file");
    return false;
  }

  for (auto &entry : instance.marked_entires_) {
    instance.delete_entry_without_lock(entry);
  }

  instance.marked_entires_.clear();
  return true;
}

void FileManager::clear_marked_entries() {
  std::unique_lock lock(file_mutex_);
  instance().marked_entires_.clear();
}

bool FileManager::delete_entry_without_lock(fs::directory_entry &entry) {
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

ftxui::Element FileManager::directory_preview(const int &selected) {
  auto &instance = FileManager::instance();

  std::unique_lock lock{file_mutex_};
  instance.update_preview_entries_without_lock(selected);
  auto entries =
      instance.preview_entries_ |
      std::views::transform([&instance](fs::directory_entry entry) {
        if (entry.path().empty() || !fs::exists(entry)) {
          return ftxui::text("[Invalid Entry]");
        }
        return ftxui::text(
            instance.format_directory_entries_without_lock(std::move(entry)));
      }) |
      std::ranges::to<std::vector>();
  if (entries.empty()) {
    entries.push_back(ftxui::text("[Empty folder]"));
  }

  return ftxui::vbox(std::move(entries));
}

std::string FileManager::text_preview(const int &selected, size_t max_lines,
                                      size_t max_width) {
  auto entry = selected_entry(selected);
  std::ifstream file(entry.value().path());

  std::ostringstream oss;
  std::string line;
  size_t lines = 0;

  while (std::getline(file, line) && lines < max_lines) {

    if (line.length() > max_width) {
      line = line.substr(0, max_width - 3) + "...";
    }

    for (auto &c : line) {
      if (iscntrl(static_cast<unsigned char>(c))) {
        c = '?';
      }
    }

    oss << line << '\n';
    ++lines;
  }
  return oss.str();
}

std::string
FileManager::entry_name_with_icon(const fs::directory_entry &entry) {
  if (entry.path().empty()) {
    return "[Invalid Entry]";
  }

  static const std::unordered_map<std::string, std::string> extension_icons{
      {".txt", "\uf15c"}, {".md", "\ueeab"},   {".cpp", "\ue61d"},
      {".hpp", "\uf0fd"}, {".h", "\uf0fd"},    {".c", "\ue61e"},
      {".jpg", "\uf4e5"}, {".jpeg", "\uf4e5"}, {".png", "\uf4e5"},
      {".gif", "\ue60d"}, {".pdf", "\ue67d"},  {".zip", "\ue6aa"},
      {".mp3", "\uf001"}, {".mp4", "\uf03d"},  {".json", "\ue60b"},
      {".log", "\uf4ed"}, {".csv", "\ueefc"},
  };

  if (!fs::exists(entry) || entry.path().empty()) {
    return "";
  }

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

std::string FileManager::format_directory_entries_without_lock(
    const fs::directory_entry &entry) const {
  std::string selected_marker = "  ";
  if (std::ranges::find(marked_entires_, entry) != marked_entires_.end()) {
    selected_marker = "█ ";
  }
  return selected_marker + instance().entry_name_with_icon(entry);
}

FileManager::Lru::Lru(size_t capacity) : capacity_(capacity) {}

void FileManager::Lru::touch(const fs::path &path) {
  lru_list_.splice(lru_list_.begin(), lru_list_, map_[path]);
}

std::optional<std::vector<fs::directory_entry>>
FileManager::Lru::get(const fs::path &path) {
  auto it = map_.find(path);
  if (it == map_.end()) {
    return std::nullopt;
  }

  touch(path);

  return cache_[path];
}

void FileManager::Lru::insert(const fs::path &path,
                              const std::vector<fs::directory_entry> &data) {
  auto it = map_.find(path);

  if (it != map_.end()) {
    cache_[path] = data;
    touch(path);
  } else {
    if (lru_list_.size() == capacity_) {
      const fs::path &lru_path = lru_list_.back();

      map_.erase(lru_path);
      cache_.erase(lru_path);

      lru_list_.pop_back();
    }

    lru_list_.push_front(path);
    map_[path] = lru_list_.begin();
    cache_[path] = data;
  }
}

} // namespace duck
