#include "file_manager.h"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
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
      is_yanking_{false}, is_renaming_{false}, show_hidden_{false} {

  curdir_entries_ = std::move(
      load_directory_entries_without_lock(current_path_, false, true));

  if (fs::is_directory(curdir_entries_[0])) {
    preview_entries_ = std::move(load_directory_entries_without_lock(
        curdir_entries_[0], show_hidden_, true));
  }

  clipboard_entries_.reserve(64);
}

FileManager &FileManager::instance() {
  static FileManager instance;
  return instance;
}

const fs::path &FileManager::current_path() {
  std::shared_lock lock{file_manager_mutex_};
  return instance().current_path_;
}

const fs::path &FileManager::cur_parent_path() {
  std::shared_lock lock{file_manager_mutex_};
  return instance().parent_path_;
}

const fs::path &FileManager::previous_path() {
  std::shared_lock lock{file_manager_mutex_};
  return instance().previous_path_;
}

const std::vector<fs::directory_entry> &FileManager::curdir_entries() {
  std::shared_lock lock{file_manager_mutex_};
  return instance().curdir_entries_;
}

const std::vector<fs::directory_entry> &FileManager::preview_entries() {
  std::shared_lock lock{file_manager_mutex_};
  return instance().preview_entries_;
}

const std::set<fs::directory_entry> &FileManager::marked_entries() {
  std::shared_lock lock{file_manager_mutex_};
  return instance().marked_entires_;
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
  std::shared_lock lock{file_manager_mutex_};
  return instance().is_yanking_;
}

bool FileManager::cutting() {
  std::shared_lock lock{file_manager_mutex_};
  return instance().is_renaming_;
}

std::expected<fs::directory_entry, std::string>
FileManager::selected_entry(const int &selected) {
  std::shared_lock lock{file_manager_mutex_};
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
                                                 bool show_hidden,
                                                 bool reload) {
  if (!fs::is_directory(path)) {
    return {};
  }

  std::vector<fs::directory_entry> entries;

  auto cache = std::move(lru_cache_.get(path));
  if (cache.has_value() && reload) {
    entries = std::move(cache.value());
    return entries;
  }

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
    (entry.is_directory() ? dirs : files).push_back(std::move(entry));
  }

  std::ranges::sort(dirs);
  std::ranges::sort(files);
  std::ranges::move(dirs, std::back_inserter(entries));
  std::ranges::move(files, std::back_inserter(entries));

  lru_cache_.insert(path, entries);

  return entries;
}

std::vector<fs::directory_entry>
FileManager::update_curdir_entries(bool use_cache) {
  auto &instance = FileManager::instance();
  fs::path target_path{};
  bool show_hidden{};

  {
    std::shared_lock lock{file_manager_mutex_};
    target_path = instance.current_path_;
    show_hidden = instance.show_hidden_;
  }

  auto entries = std::move(instance.load_directory_entries_without_lock(
      target_path, show_hidden, use_cache));

  {
    std::unique_lock lock{file_manager_mutex_};
    instance.curdir_entries_ = std::move(entries);
    return instance.curdir_entries_;
  }
}

void FileManager::update_current_path(const fs::path &new_path) {
  {
    std::unique_lock lock{FileManager::file_manager_mutex_};
    auto &instance = FileManager::instance();
    instance.previous_path_ = instance.current_path_;
    instance.current_path_ = new_path;
    instance.parent_path_ = instance.current_path_.parent_path();
  }
}

void FileManager::toggle_mark_on_selected(const int &selected) {
  std::unique_lock lock{file_manager_mutex_};
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
    instance.marked_entires_.insert(instance.curdir_entries_[selected]);
  }
}

void FileManager::toggle_hidden_entries() {
  std::unique_lock lock{file_manager_mutex_};
  instance().show_hidden_ = !instance().show_hidden_;
}

void FileManager::start_yanking() {
  std::unique_lock lock{file_manager_mutex_};
  instance().is_yanking_ = true;
  instance().is_renaming_ = false;
}

void FileManager::start_cutting() {
  std::unique_lock lock{file_manager_mutex_};
  instance().is_renaming_ = true;
  instance().is_yanking_ = false;
}

fs::path FileManager::get_dest_path(const fs::directory_entry &entry,
                                    const fs::path &current_path) {
  fs::path dest_path = current_path / entry.path().filename();
  int cnt{1};
  auto file_name = entry.path().filename().string();
  while (fs::exists(dest_path)) {
    dest_path = current_path / (file_name + std::format("_{}", cnt));
    cnt++;
  }
  return dest_path;
}

void FileManager::yank_entries(const std::vector<fs::directory_entry> &entries,
                               const fs::path &current_path) {
  for (const auto &entry : entries) {
    fs::copy(entry.path(), get_dest_path(entry, current_path),
             fs::copy_options::recursive);
  }
}

void FileManager::rename_entries(
    const std::vector<fs::directory_entry> &entries,
    const fs::path &current_path) {
  for (const auto &entry : entries) {
    fs::rename(entry.path(), get_dest_path(entry, current_path));
  }
}

void FileManager::yank_or_rename(const int &selected) {
  auto &instance = FileManager::instance();
  if (!instance.is_renaming_ && !instance.is_yanking_) {
    return;
  }
  std::vector<fs::directory_entry> entries;
  fs::path current_path;
  bool is_renaming;

  {
    std::unique_lock lock{file_manager_mutex_};
    if (instance.marked_entires_.empty()) {
      entries.push_back(instance.curdir_entries_[selected]);
    } else {
      entries = std::move(std::vector<fs::directory_entry>{
          instance.marked_entires_.begin(), instance.marked_entires_.end()});
    }
    instance.marked_entires_.clear();
    current_path = instance.current_path_;
    is_renaming = instance.is_renaming_;
  }
  if (is_renaming) {
    rename_entries(entries, current_path);
  } else {
    yank_entries(entries, current_path);
  }
}

bool FileManager::is_marked(const fs::directory_entry &entry) {
  std::shared_lock lock(file_manager_mutex_);
  return instance().marked_entires_.contains(entry);
}

bool FileManager::delete_selected_entry(const int &selected) {
  std::unique_lock lock{file_manager_mutex_};
  return instance().delete_entry_without_lock(
      instance().curdir_entries_[selected]);
}

void FileManager::rename_selected_entry(const int &selected,
                                        std::string new_name) {

  fs::directory_entry entry;
  fs::path cur_path;
  {
    std::shared_lock lock{file_manager_mutex_};
    entry = instance().curdir_entries_[selected];
    cur_path = instance().current_path_;
  }

  fs::path dest_path = cur_path / new_name;
  int cnt{1};
  while (fs::exists(dest_path)) {
    dest_path = cur_path / (new_name + std::format("_{}", cnt));
    cnt++;
  }
  fs::rename(entry, dest_path);
}

bool FileManager::delete_marked_entries() {

  std::unique_lock lock(file_manager_mutex_);
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
  std::unique_lock lock(file_manager_mutex_);
  instance().marked_entires_.clear();
}

bool FileManager::delete_entry_without_lock(const fs::directory_entry &entry) {
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

std::vector<fs::directory_entry>
FileManager::directory_preview(const std::pair<int, int> &selected_and_size) {
  fs::path target_path;
  bool show_hidden{};
  auto [selected, preview_size] = selected_and_size;

  {
    auto &instance = FileManager::instance();
    std::shared_lock lock{file_manager_mutex_};
    if (instance.curdir_entries_.empty()) {
      return {};
    }
    if (selected < 0 || selected >= instance.curdir_entries_.size()) {
      return {};
    }
    if (not fs::is_directory(instance.curdir_entries_[selected])) {
      return {};
    }
    target_path = instance.curdir_entries_[selected].path();
    show_hidden = instance.show_hidden_;
  }

  auto entries = std::move(instance().load_directory_entries_without_lock(
      target_path, show_hidden, true));
  {
    std::unique_lock lock{file_manager_mutex_};
    instance().preview_entries_.assign(
        std::make_move_iterator(entries.begin()),
        std::make_move_iterator(
            entries.begin() +
            std::min(preview_size, static_cast<int>(entries.size()))));

    return instance().preview_entries_;
  }
}

std::string FileManager::text_preview(const int &selected,
                                      std::pair<int, int> size) {
  auto [max_width, max_lines] = size;
  max_width /= 2;
  max_width += 3;
  const auto entry = selected_entry(selected);
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
  return selected_marker + FileManager::entry_name_with_icon(entry);
}

ftxui::Element
FileManager::entries_string_to_element(std::vector<std::string> entries,
                                       int selected) {
  auto result = entries | std::views::transform([](std::string &entry) {
                  return ftxui::text(std::move(entry));
                }) |
                std::ranges::to<std::vector>();
  if (result.empty()) {
    result.push_back(ftxui::text("[Empty folder]"));
  } else if (selected >= 0 && selected < static_cast<int>(result.size())) {
    result[selected] |= ftxui::bold | ftxui::color(ftxui::Color::Black) |
                        ftxui::bgcolor(ftxui::Color::RGB(186, 187, 241));
  }
  return ftxui::vbox(std::move(result));
}

ftxui::Element
FileManager::entries_string_to_element(std::vector<std::string> entries) {
  auto result = entries | std::views::transform([](std::string &entry) {
                  return ftxui::text(std::move(entry));
                }) |
                std::ranges::to<std::vector>();
  if (result.empty()) {
    result.push_back(ftxui::text("[Empty folder]"));
  }
  return ftxui::vbox(std::move(result));
}

std::vector<std::string>
FileManager::format_entries(const std::vector<fs::directory_entry> &entries) {
  auto &instance = FileManager::instance();
  if (entries.empty()) {
    return std::vector<std::string>{"[No items]"};
  }
  std::vector<std::string> entries_string{};
  entries_string.reserve(entries.size());

  std::shared_lock lock{file_manager_mutex_};
  for (auto &entry : entries) {
    entries_string.push_back(
        instance.format_directory_entries_without_lock(entry));
  }
  return entries_string;
}

template <typename Key, typename Value>
FileManager::Lru<Key, Value>::Lru(size_t capacity) : capacity_(capacity) {}

template <typename Key, typename Value>
void FileManager::Lru<Key, Value>::touch_without_lock(const Key &path) {
  lru_list_.splice(lru_list_.begin(), lru_list_, map_[path]);
}

template <typename Key, typename Value>
std::optional<Value> FileManager::Lru<Key, Value>::get(const Key &path) {

  std::unique_lock lock{lru_mutex_};
  auto it = map_.find(path);
  if (it == map_.end()) {
    return std::nullopt;
  }

  touch_without_lock(path);

  return cache_[path];
}

template <typename Key, typename Value>
void FileManager::Lru<Key, Value>::insert(const Key &path, const Value &data) {
  auto it = map_.find(path);
  std::unique_lock lock{lru_mutex_};
  if (it != map_.end()) {
    cache_[path] = data;
    touch_without_lock(path);
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
