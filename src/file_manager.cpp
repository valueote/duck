#include "file_manager.hpp"
#include "app_state.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <vector>

namespace duck {

constexpr size_t clipboard_reserve = 50;
constexpr size_t dirs_reserve = 256;
constexpr size_t files_reserve = 256;

void FileManagerService::init(AppState &state) {
  state.current_path_ = fs::current_path();

  load_directory_entries(state, state.current_path_, true);
  state.cache.get(state.current_path_)
      .and_then([&state](const auto &direcotry) -> std::optional<Direcotry> {
        state.current_direcotry_ = direcotry;
        if (const auto &entries = direcotry.entries_; !entries.empty()) {
          load_directory_entries(state, entries[0].path(), true);
        }
        return std::nullopt;
      });
}

void FileManagerService::load_directory_entries(AppState &state,
                                                const fs::path &path,
                                                bool use_cache) {
  if (!fs::is_directory(path)) {
    return;
  }

  Direcotry direcotry;

  if (auto cache = state.cache.get(path); cache.has_value() && use_cache) {
    direcotry = cache.value();
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
  state.cache.insert(path, direcotry);
}

std::vector<fs::directory_entry>
FileManagerService::get_entries(AppState &state, const fs::path &target_path,
                                bool show_hidden) {
  return state.cache.get(target_path)
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

std::vector<fs::directory_entry>
FileManagerService::curdir_entries(AppState &state) {
  return get_entries(state, state.current_path_, state.show_hidden_);
}

int FileManagerService::previous_path_index(AppState &state) {
  auto entries = curdir_entries(state);
  if (auto iter = std::ranges::find(entries, state.previous_path_);
      iter != entries.end()) {
    return static_cast<int>(std::distance(entries.begin(), iter));
  }
  return 0;
}

std::optional<fs::directory_entry>
FileManagerService::selected_entry(AppState &state) {
  auto entries = curdir_entries(state);

  if (entries.empty()) {
    return std::nullopt;
  }

  if (state.index < 0 || state.index >= entries.size()) {
    return std::nullopt;
  }

  return entries[state.index];
}

std::vector<fs::directory_entry>
FileManagerService::update_curdir_entries(AppState &state, bool use_cache) {
  load_directory_entries(state, state.current_path_, use_cache);
  return curdir_entries(state);
}

void FileManagerService::update_current_path(AppState &state,
                                             const fs::path &new_path) {
  state.previous_path_ = state.current_path_;
  state.current_path_ = new_path;
}

void FileManagerService::toggle_mark_on_selected(AppState &state) {
  auto entries = curdir_entries(state);

  if (entries.empty() || state.index < 0 || state.index >= entries.size()) {
    return;
  }

  if (std::ranges::find_if(
          state.selected_entries_,
          [&state, &entries](const fs::directory_entry &entry) {
            return entry.path() == entries[state.index].path().parent_path();
          }) != state.selected_entries_.end()) {
    return;
  }

  if (auto iter =
          std::ranges::find(state.selected_entries_, entries[state.index]);
      iter != state.selected_entries_.end()) {
    state.selected_entries_.erase(iter);
  } else {
    state.selected_entries_.insert(entries[state.index]);
  }
}

void FileManagerService::toggle_hidden_entries(AppState &state) {
  state.show_hidden_ = !state.show_hidden_;
}

void FileManagerService::start_yanking(AppState &state) {
  state.is_yanking_ = true;
  state.is_cutting_ = false;

  if (state.selected_entries_.empty()) {
    selected_entry(state).transform([&state](const auto &entry) {
      state.selected_entries_.insert(entry);
      return entry;
    });
  }
}

void FileManagerService::start_cutting(AppState &state) {
  state.is_cutting_ = true;
  state.is_yanking_ = false;
  if (state.selected_entries_.empty()) {
    selected_entry(state).transform([&state](const auto &entry) {
      state.selected_entries_.insert(entry);
      return entry;
    });
  }
}

void FileManagerService::yank_or_cut(AppState &state) {
  if (!state.is_cutting_ && !state.is_yanking_) {
    return;
  }
  std::vector<fs::directory_entry> entries;

  if (state.selected_entries_.empty()) {
    selected_entry(state).transform([&entries](const auto &entry) {
      entries.push_back(entry);
      return entry;
    });
  } else {
    entries = std::move(std::vector<fs::directory_entry>{
        state.selected_entries_.begin(), state.selected_entries_.end()});
  }
  state.selected_entries_.clear();

  if (state.is_cutting_) {
    cut_entries(state);
  } else {
    yank_entries(state);
  }
}

bool FileManagerService::is_marked(AppState &state,
                                   const fs::directory_entry &entry) {
  return state.selected_entries_.contains(entry);
}

bool FileManagerService::delete_selected_entry(AppState &state) {
  return selected_entry(state)
      .transform([](const auto &entry) { return delete_entry(entry); })
      .value_or(false);
}

void FileManagerService::rename_selected_entry(AppState &state,
                                               const std::string &new_name) {
  fs::path dest_path = state.current_path_ / new_name;
  int cnt{1};
  while (fs::exists(dest_path)) {
    dest_path = state.current_path_ / (new_name + std::format("_{}", cnt));
    cnt++;
  }

  selected_entry(state).transform([dest_path](const auto &entry) {
    fs::rename(entry, dest_path);
    return 0;
  });
}

bool FileManagerService::delete_marked_entries(AppState &state) {
  if (state.selected_entries_.empty()) {
    std::println(stderr, "[ERROR] try to delete empty file");
    return false;
  }

  for (const auto &entry : state.selected_entries_) {
    FileManagerService::delete_entry(entry);
  }

  state.selected_entries_.clear();
  return true;
}

void FileManagerService::create_new_entry(AppState &state,
                                          const std::string &filename) {
  if (filename.empty()) {
    return;
  }
  auto dest_path = state.current_path_ / filename;

  if (filename.ends_with('/')) {
    fs::create_directories(dest_path);
  } else {
    std::ofstream ofs(dest_path);
    ofs.close();
  }
}

void FileManagerService::clear_marked_entries(AppState &state) {
  if (state.is_yanking_ || state.is_cutting_) {
    state.is_yanking_ = false;
    state.is_cutting_ = false;
  } else {
    state.selected_entries_.clear();
  }
}

void FileManagerService::directory_preview(
    AppState &state, const std::pair<int, int> &selected_and_size) {
  auto [selected, preview_size] = selected_and_size;

  auto selected_entry_opt = selected_entry(state);
  if (!selected_entry_opt.has_value() ||
      !fs::is_directory(selected_entry_opt.value())) {
    state.entries_preview = ftxui::text("");
    return;
  }

  auto target_path = selected_entry_opt.value().path();
  load_directory_entries(state, target_path, true);
  auto entries = get_entries(state, target_path, state.show_hidden_);

  state.entries_preview = ftxui::vbox(entries | std::views::take(preview_size) |
                                      std::views::transform([](const auto &e) {
                                        return ftxui::text(e.path().string());
                                      }));
}

std::string FileManagerService::text_preview(AppState &state,
                                             std::pair<int, int> size) {
  auto [max_width, max_lines] = size;
  max_width /= 2;
  max_width += 3;
  const auto entry = selected_entry(state);
  if (!entry.has_value()) {
    return "";
  }
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

bool FileManagerService::entries_sorter(const fs::directory_entry &first,
                                        const fs::directory_entry &second) {
  if (first.is_directory() != second.is_directory()) {
    return static_cast<int>(first.is_directory()) >
           static_cast<int>(second.is_directory());
  }

  return first.path().filename() < second.path().filename();
}

fs::path FileManagerService::get_dest_path(const fs::directory_entry &entry,
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

bool FileManagerService::delete_entry(const fs::directory_entry &entry) {
  if (!entry.exists()) {
    return false;
  }

  if (fs::is_directory(entry)) {
    return fs::remove_all(entry) != 0U;
  }

  return fs::remove(entry);
}

void FileManagerService::yank_entries(AppState &state) {
  for (const auto &entry : state.selected_entries_) {
    fs::copy(entry.path(), get_dest_path(entry, state.current_path_),
             fs::copy_options::recursive);
  }
}

void FileManagerService::cut_entries(AppState &state) {
  for (const auto &entry : state.selected_entries_) {
    fs::rename(entry.path(), get_dest_path(entry, state.current_path_));
  }
}

} // namespace duck
