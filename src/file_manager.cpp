#include "file_manager.hpp"
#include <filesystem>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <iterator>
#include <mutex>
#include <optional>
#include <print>
#include <shared_mutex>
#include <vector>

namespace duck {

constexpr size_t lru_cache_size = 50;
constexpr size_t clipboard_reserve = 50;
constexpr size_t dirs_reserve = 256;
constexpr size_t files_reserve = 256;

FileManager::FileManager()
    : current_path_{fs::current_path()},
      parent_path_{current_path_.parent_path()}, cache_{lru_cache_size},
      is_yanking_{false}, is_cutting_{false}, show_hidden_{false} {

  load_directory_entries(current_path_, true);

  cache_.get(current_path_)
      .and_then([this](const auto &direcotry) -> std::optional<Direcotry> {
        if (const auto &entries = direcotry.entries_; !entries.empty()) {
          load_directory_entries(entries[0].path(), true);
        }
        return std::nullopt;
      });

  clipboard_entries_.reserve(clipboard_reserve);
}

void FileManager::load_directory_entries(const fs::path &path, bool use_cache) {
  if (!fs::is_directory(path)) {
    return;
  }

  Direcotry direcotry;

  if (auto cache = std::move(cache_.get(path));
      cache.has_value() && use_cache) {
    direcotry = std::move(cache.value());
    return;
  }

  direcotry.entries_.reserve(dirs_reserve);
  direcotry.hidden_entries_.reserve(dirs_reserve);

  for (auto entry : fs::directory_iterator(
           path, fs::directory_options::skip_permission_denied)) {
    if (entry.path().empty()) {
      continue;
    }
    if (entry.path().filename().native()[0] == '.') {
      direcotry.hidden_entries_.push_back(std::move(entry));
      continue;
    }
    direcotry.entries_.push_back(std::move(entry));
  }

  std::ranges::sort(direcotry.entries_, entries_sorter);
  cache_.insert(path, direcotry);
}

const fs::path &FileManager::current_path() {
  std::shared_lock lock{file_manager_mutex_};
  return current_path_;
}

const fs::path &FileManager::cur_parent_path() {
  std::shared_lock lock{file_manager_mutex_};
  return parent_path_;
}

const fs::path &FileManager::previous_path() {
  std::shared_lock lock{file_manager_mutex_};
  return previous_path_;
}

std::vector<fs::directory_entry>
FileManager::get_entries(const fs::path &target_path, bool show_hidden) {
  return cache_.get(target_path)
      .transform([show_hidden](const auto &direcotry)
                     -> std::vector<fs::directory_entry> {
        auto entries = direcotry.entries_;
        if (show_hidden) {
          entries.reserve(entries.size() + direcotry.hidden_entries_.size());
          std::ranges::copy(direcotry.hidden_entries_,
                            std::back_inserter(entries));
          std::ranges::sort(entries, entries_sorter);
        }
        return entries;
      })
      .value_or({});
}

std::vector<fs::directory_entry> FileManager::curdir_entries() {
  fs::path current_path{};
  bool show_hidden{false};

  {
    std::shared_lock lock{file_manager_mutex_};
    current_path = current_path_;
    show_hidden = show_hidden_;
  }
  return get_entries(current_path, show_hidden);
}

std::vector<fs::directory_entry> FileManager::preview_entries() {
  std::shared_lock lock{file_manager_mutex_};
  return preview_entries_;
}

std::set<fs::directory_entry> FileManager::marked_entries() {
  std::shared_lock lock{file_manager_mutex_};
  return marked_entries_;
}

int FileManager::previous_path_index() {
  fs::path current_path{};
  fs::path previous_path{};
  {
    std::shared_lock lock{file_manager_mutex_};
    current_path = current_path_;
    previous_path = previous_path_;
  }

  auto curdir_entries = FileManager::curdir_entries();
  if (auto iter = std::ranges::find(curdir_entries, previous_path);
      iter != curdir_entries.end()) {
    return static_cast<int>(std::distance(curdir_entries.begin(), iter));
  }
  return 0;
}

bool FileManager::yanking() {
  std::shared_lock lock{file_manager_mutex_};
  return is_yanking_;
}

bool FileManager::cutting() {
  std::shared_lock lock{file_manager_mutex_};
  return is_cutting_;
}

std::optional<fs::directory_entry>
FileManager::selected_entry(const int &selected) {
  auto entries = curdir_entries();

  if (entries.empty()) {
    return std::nullopt;
  }

  if (selected < 0 || selected >= entries.size()) {
    return std::nullopt;
  }

  return entries[selected];
}

std::vector<fs::directory_entry>
FileManager::update_curdir_entries(bool use_cache) {
  fs::path current_path{};
  bool show_hidden{};

  {
    std::shared_lock lock{file_manager_mutex_};
    current_path = current_path_;
    show_hidden = show_hidden_;
  }
  load_directory_entries(current_path, use_cache);

  return curdir_entries();
}

void FileManager::update_current_path(const fs::path &new_path) {
  std::unique_lock lock{file_manager_mutex_};
  previous_path_ = current_path_;
  current_path_ = new_path;
  parent_path_ = current_path_.parent_path();
}

void FileManager::toggle_mark_on_selected(const int selected) {
  std::unique_lock lock{file_manager_mutex_};

  auto entries = curdir_entries();

  if (entries.empty() || selected < 0 || selected >= entries.size()) {
    return;
  }

  if (std::ranges::find_if(
          marked_entries_,
          [&selected, &entries](const fs::directory_entry &entry) {
            return entry.path() == entries[selected].path().parent_path();
          }) != marked_entries_.end()) {
    return;
  }

  if (auto iter = std::ranges::find(marked_entries_, entries[selected]);
      iter != marked_entries_.end()) {
    marked_entries_.erase(iter);
  } else {
    marked_entries_.insert(entries[selected]);
  }
}

void FileManager::toggle_hidden_entries() {
  std::unique_lock lock{file_manager_mutex_};
  show_hidden_ = !show_hidden_;
}

void FileManager::start_yanking(const int selected) {

  std::unique_lock lock{file_manager_mutex_};
  is_yanking_ = true;
  is_cutting_ = false;

  if (marked_entries_.empty()) {
    selected_entry(selected).transform([this](const auto &entry) {
      marked_entries_.insert(entry);
      return entry;
    });
  }
}

void FileManager::start_cutting(const int selected) {
  std::unique_lock lock{file_manager_mutex_};
  is_cutting_ = true;
  is_yanking_ = false;
  if (marked_entries_.empty()) {
    selected_entry(selected).transform([this](const auto &entry) {
      marked_entries_.insert(entry);
      return entry;
    });
  }
}

void FileManager::yank_or_cut(const int selected) {
  if (!is_cutting_ && !is_yanking_) {
    return;
  }
  std::vector<fs::directory_entry> entries;
  fs::path current_path;
  bool is_cutting{false};

  {
    std::unique_lock lock{file_manager_mutex_};
    if (marked_entries_.empty()) {
      selected_entry(selected).transform([&entries](const auto &entry) {
        entries.push_back(entry);
        return entry;
      });
    } else {
      entries = std::move(std::vector<fs::directory_entry>{
          marked_entries_.begin(), marked_entries_.end()});
    }
    marked_entries_.clear();
    current_path = current_path_;
    is_cutting = is_cutting_;
  }

  if (is_cutting) {
    cut_entries(entries, current_path);
  } else {
    yank_entries(entries, current_path);
  }
}

bool FileManager::is_marked(const fs::directory_entry &entry) {
  std::shared_lock lock(file_manager_mutex_);
  return marked_entries_.contains(entry);
}

bool FileManager::delete_selected_entry(const int selected) {
  std::unique_lock lock{file_manager_mutex_};
  return selected_entry(selected)
      .transform([this](const auto &entry) { return delete_entry(entry); })
      .value();
}

void FileManager::rename_selected_entry(const int selected,
                                        const std::string &new_name) {
  fs::path cur_path;

  {
    std::shared_lock lock{file_manager_mutex_};
    cur_path = current_path_;
  }

  fs::path dest_path = cur_path / new_name;
  int cnt{1};
  while (fs::exists(dest_path)) {
    dest_path = cur_path / (new_name + std::format("_{}", cnt));
    cnt++;
  }

  selected_entry(selected).transform([dest_path](const auto &entry) {
    fs::rename(entry, dest_path);
    return 0;
  });
}

bool FileManager::delete_marked_entries() {
  std::unique_lock lock(file_manager_mutex_);
  if (marked_entries_.empty()) {
    std::println(stderr, "[ERROR] try to delete empty file");
    return false;
  }

  for (const auto &entry : marked_entries_) {
    FileManager::delete_entry(entry);
  }

  marked_entries_.clear();
  return true;
}

void FileManager::create_new_entry(const std::string &filename) {
  if (filename.empty()) {
    return;
  }
  std::shared_lock lock{file_manager_mutex_};
  auto dest_path = current_path_ / filename;

  if (filename.ends_with('/')) {
    fs::create_directories(dest_path);
  } else {
    std::ofstream ofs(dest_path);
    ofs.close();
  }
}

void FileManager::clear_marked_entries() {
  std::unique_lock lock(file_manager_mutex_);
  if (is_yanking_ || is_cutting_) {
    is_yanking_ = false;
    is_cutting_ = false;
  } else {
    marked_entries_.clear();
  }
}

std::vector<fs::directory_entry>
FileManager::directory_preview(const std::pair<int, int> &selected_and_size) {
  fs::path target_path;
  bool show_hidden{};
  auto [selected, preview_size] = selected_and_size;

  {
    std::shared_lock lock{file_manager_mutex_};
    show_hidden = show_hidden_;
  }

  auto selected_entry = FileManager::selected_entry(selected).value_or({});
  if (not selected_entry.exists() || not fs::is_directory(selected_entry)) {
    return {};
  }
  target_path = selected_entry.path();

  load_directory_entries(target_path, true);
  auto entries = get_entries(target_path, show_hidden);

  std::unique_lock lock{file_manager_mutex_};
  preview_entries_.assign(
      std::make_move_iterator(entries.begin()),
      std::make_move_iterator(
          entries.begin() +
          std::min(preview_size, static_cast<int>(entries.size()))));

  return preview_entries_;
}

std::string FileManager::text_preview(const int selected,
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

    for (auto &character : line) {
      if (iscntrl(static_cast<unsigned char>(character)) != 0) {
        character = '?';
      }
    }

    oss << line << '\n';
    ++lines;
  }
  return oss.str();
}

bool FileManager::entries_sorter(const fs::directory_entry &first,
                                 const fs::directory_entry &second) {
  if (first.is_directory() != second.is_directory()) {
    return static_cast<int>(first.is_directory()) >
           static_cast<int>(second.is_directory());
  }

  return first.path().filename() < second.path().filename();
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

bool FileManager::delete_entry(const fs::directory_entry &entry) {
  if (!entry.exists()) {
    return false;
  }

  if (fs::is_directory(entry)) {
    return fs::remove_all(entry) != 0U;
  }

  return fs::remove(entry);
}

void FileManager::yank_entries(const std::vector<fs::directory_entry> &entries,
                               const fs::path &current_path) {
  for (const auto &entry : entries) {
    fs::copy(entry.path(), get_dest_path(entry, current_path),
             fs::copy_options::recursive);
  }
}

void FileManager::cut_entries(const std::vector<fs::directory_entry> &entries,
                              const fs::path &current_path) {
  for (const auto &entry : entries) {
    fs::rename(entry.path(), get_dest_path(entry, current_path));
  }
}

} // namespace duck
