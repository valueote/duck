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

FileManager &FileManager::instance() {
  static FileManager instance;
  return instance;
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

bool FileManager::delete_entry(const fs::directory_entry &entry) {
  if (!entry.exists()) {
    return false;
  }

  if (fs::is_directory(entry)) {
    return fs::remove_all(entry) != 0U;
  }

  return fs::remove(entry);
}

bool FileManager::entries_sorter(const fs::directory_entry &first,
                                 const fs::directory_entry &second) {
  if (first.is_directory() != second.is_directory()) {
    return static_cast<int>(first.is_directory()) >
           static_cast<int>(second.is_directory());
  }

  return first.path().filename() < second.path().filename();
}

const fs::path &FileManager::current_path() {
  std::shared_lock lock{instance().file_manager_mutex_};
  return instance().current_path_;
}

const fs::path &FileManager::cur_parent_path() {
  std::shared_lock lock{instance().file_manager_mutex_};
  return instance().parent_path_;
}

const fs::path &FileManager::previous_path() {
  std::shared_lock lock{instance().file_manager_mutex_};
  return instance().previous_path_;
}

std::vector<fs::directory_entry>
FileManager::get_entries(const fs::path &target_path, bool show_hidden) {
  return instance()
      .cache_.get(target_path)
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
  auto &instance = FileManager::instance();
  bool show_hidden{false};

  {
    std::shared_lock lock{instance.file_manager_mutex_};
    current_path = instance.current_path_;
    show_hidden = instance.show_hidden_;
  }
  return get_entries(current_path, show_hidden);
}

std::vector<fs::directory_entry> FileManager::preview_entries() {
  std::shared_lock lock{instance().file_manager_mutex_};
  return instance().preview_entries_;
}

std::set<fs::directory_entry> FileManager::marked_entries() {
  std::shared_lock lock{instance().file_manager_mutex_};
  return instance().marked_entries_;
}

int FileManager::previous_path_index() {
  fs::path current_path{};
  fs::path previous_path{};
  auto &instance = FileManager::instance();
  {
    std::shared_lock lock{instance.file_manager_mutex_};
    current_path = instance.current_path_;
    previous_path = instance.previous_path_;
  }

  auto curdir_entries = FileManager::curdir_entries();
  if (auto iter = std::ranges::find(curdir_entries, previous_path);
      iter != curdir_entries.end()) {
    return static_cast<int>(std::distance(curdir_entries.begin(), iter));
  }
  return 0;
}

bool FileManager::yanking() {
  std::shared_lock lock{instance().file_manager_mutex_};
  return instance().is_yanking_;
}

bool FileManager::cutting() {
  std::shared_lock lock{instance().file_manager_mutex_};
  return instance().is_cutting_;
}

std::optional<fs::directory_entry>
FileManager::selected_entry(const int &selected) {
  auto curdir_entries = FileManager::curdir_entries();

  if (curdir_entries.empty()) {
    return std::nullopt;
  }

  if (selected < 0 || selected >= curdir_entries.size()) {
    return std::nullopt;
  }

  return curdir_entries[selected];
}

std::vector<fs::directory_entry>
FileManager::update_curdir_entries(bool use_cache) {
  auto &instance = FileManager::instance();
  fs::path current_path{};
  bool show_hidden{};

  {
    std::shared_lock lock{instance.file_manager_mutex_};
    current_path = instance.current_path_;
    show_hidden = instance.show_hidden_;
  }
  instance.load_directory_entries(current_path, use_cache);

  return instance.cache_.get(current_path)
      .transform([show_hidden](const auto &direcotry)
                     -> std::vector<fs::directory_entry> {
        auto entries = direcotry.entries_;
        if (show_hidden) {
          std::ranges::copy(direcotry.hidden_entries_,
                            std::back_inserter(entries));
          std::ranges::sort(entries, entries_sorter);
        }
        return entries;
      })
      .value_or({});
}

void FileManager::update_current_path(const fs::path &new_path) {
  auto &instance = FileManager::instance();
  std::unique_lock lock{instance.file_manager_mutex_};
  instance.previous_path_ = instance.current_path_;
  instance.current_path_ = new_path;
  instance.parent_path_ = instance.current_path_.parent_path();
}

void FileManager::toggle_mark_on_selected(const int selected) {
  auto &instance = FileManager::instance();
  std::unique_lock lock{instance.file_manager_mutex_};

  auto curdir_entries =
      instance.cache_.get(instance.current_path_)
          .transform([](const auto &direcotry) { return direcotry.entries_; })
          .value_or({});

  if (curdir_entries.empty() || selected < 0 ||
      selected >= curdir_entries.size()) {
    return;
  }

  if (std::ranges::find_if(
          instance.marked_entries_, [&instance, selected, &curdir_entries](
                                        const fs::directory_entry &entry) {
            return entry.path() ==
                   curdir_entries[selected].path().parent_path();
          }) != instance.marked_entries_.end()) {
    return;
  }

  if (auto iter =
          std::ranges::find(instance.marked_entries_, curdir_entries[selected]);
      iter != instance.marked_entries_.end()) {
    instance.marked_entries_.erase(iter);
  } else {
    instance.marked_entries_.insert(curdir_entries[selected]);
  }
}

void FileManager::toggle_hidden_entries() {
  std::unique_lock lock{instance().file_manager_mutex_};
  instance().show_hidden_ = !instance().show_hidden_;
}

void FileManager::start_yanking(const int selected) {
  auto &instance = FileManager::instance();

  std::unique_lock lock{instance.file_manager_mutex_};
  instance.is_yanking_ = true;
  instance.is_cutting_ = false;

  if (instance.marked_entries_.empty()) {
    instance.marked_entries_.insert(curdir_entries()[selected]);
  }
}

void FileManager::start_cutting(const int selected) {
  auto &instance = FileManager::instance();
  std::unique_lock lock{instance.file_manager_mutex_};
  instance.is_cutting_ = true;
  instance.is_yanking_ = false;
  if (instance.marked_entries_.empty()) {
    instance.marked_entries_.insert(curdir_entries()[selected]);
  }
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

void FileManager::yank_or_cut(const int selected) {
  auto &instance = FileManager::instance();
  if (!instance.is_cutting_ && !instance.is_yanking_) {
    return;
  }
  std::vector<fs::directory_entry> entries;
  fs::path current_path;
  bool is_cutting{false};

  {
    std::unique_lock lock{instance.file_manager_mutex_};
    if (instance.marked_entries_.empty()) {
      selected_entry(selected).transform([&entries](const auto &entry) {
        entries.push_back(entry);
        return entry;
      });
    } else {
      entries = std::move(std::vector<fs::directory_entry>{
          instance.marked_entries_.begin(), instance.marked_entries_.end()});
    }
    instance.marked_entries_.clear();
    current_path = instance.current_path_;
    is_cutting = instance.is_cutting_;
  }

  if (is_cutting) {
    rename_entries(entries, current_path);
  } else {
    yank_entries(entries, current_path);
  }
}

bool FileManager::is_marked(const fs::directory_entry &entry) {
  std::shared_lock lock(instance().file_manager_mutex_);
  return instance().marked_entries_.contains(entry);
}

bool FileManager::delete_selected_entry(const int selected) {
  std::unique_lock lock{instance().file_manager_mutex_};
  return delete_entry(selected_entry(selected).value_or({}));
}

void FileManager::rename_selected_entry(const int selected,
                                        const std::string &new_name) {
  fs::path cur_path;

  {
    std::shared_lock lock{instance().file_manager_mutex_};
    cur_path = instance().current_path_;
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
  std::unique_lock lock(instance().file_manager_mutex_);
  auto &instance = FileManager::instance();
  if (instance.marked_entries_.empty()) {
    std::println(stderr, "[ERROR] try to delete empty file");
    return false;
  }

  for (const auto &entry : instance.marked_entries_) {
    FileManager::delete_entry(entry);
  }

  instance.marked_entries_.clear();
  return true;
}

void FileManager::create_new_entry(const std::string &filename) {
  if (filename.empty()) {
    return;
  }
  std::shared_lock lock{instance().file_manager_mutex_};
  auto dest_path = instance().current_path_ / filename;

  if (filename.ends_with('/')) {
    fs::create_directories(dest_path);
  } else {
    std::ofstream ofs(dest_path);
    ofs.close();
  }
}

void FileManager::clear_marked_entries() {
  std::unique_lock lock(instance().file_manager_mutex_);
  auto &instance = FileManager::instance();
  if (instance.is_yanking_ || instance.is_cutting_) {
    instance.is_yanking_ = false;
    instance.is_cutting_ = false;
  } else {

    instance.marked_entries_.clear();
  }
}

std::vector<fs::directory_entry>
FileManager::directory_preview(const std::pair<int, int> &selected_and_size) {
  fs::path target_path;
  bool show_hidden{};
  auto [selected, preview_size] = selected_and_size;
  auto &instance = FileManager::instance();

  {
    std::shared_lock lock{instance.file_manager_mutex_};
    show_hidden = instance.show_hidden_;
  }

  auto selected_entry = FileManager::selected_entry(selected).value_or({});
  if (not selected_entry.exists() || not fs::is_directory(selected_entry)) {
    return {};
  }
  target_path = selected_entry.path();

  instance.load_directory_entries(target_path, true);
  auto entries = get_entries(target_path, show_hidden);

  std::unique_lock lock{instance.file_manager_mutex_};
  instance.preview_entries_.assign(
      std::make_move_iterator(entries.begin()),
      std::make_move_iterator(
          entries.begin() +
          std::min(preview_size, static_cast<int>(entries.size()))));

  return instance.preview_entries_;
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
  std::ranges::transform(ext, ext.begin(), [](char character) {
    return static_cast<char>(std::tolower(character));
  });

  auto icon_it = extension_icons.find(ext);
  const std::string &icon =
      icon_it != extension_icons.end() ? icon_it->second : "\uf15c";

  return std::format("{} {}", icon, filename);
}

} // namespace duck
