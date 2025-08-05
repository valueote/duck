#pragma once
#include <exec/task.hpp>
#include <expected>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <generator>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexec/execution.hpp>
#include <unordered_map>
#include <vector>

namespace duck {

namespace fs = std::filesystem;

class FileManager {

private:
  class Lru {
  public:
    Lru(size_t capacity);
    std::optional<std::vector<fs::directory_entry>> get(const fs::path &path);
    void insert(const fs::path &path,
                const std::vector<fs::directory_entry> &data);

  private:
    size_t capacity_;
    std::list<fs::path> lru_list_;
    std::unordered_map<fs::path, std::list<fs::path>::iterator> map_;
    std::unordered_map<fs::path, std::vector<fs::directory_entry>> cache_;

    void touch(const fs::path &path);
  };

  Lru lru_cache_;
  fs::path current_path_;
  fs::path previous_path_;
  fs::path parent_path_;
  std::vector<fs::directory_entry> curdir_entries_;
  std::vector<fs::directory_entry> hidden_entries_;
  std::vector<fs::directory_entry> preview_entries_;
  std::vector<fs::directory_entry> marked_entires_;
  std::vector<fs::directory_entry> clipboard_entries_;
  bool is_yanking_;
  bool is_cutting_;
  bool show_hidden_;

  FileManager();
  static FileManager &instance();
  std::vector<fs::directory_entry>
  load_directory_entries_without_lock(const fs::path &path, bool preview);
  std::generator<std::vector<fs::directory_entry>>
  lazy_load_directory_entries_without_lock(const fs::path &path, bool preview,
                                           const size_t &chunk = 50);
  bool delete_entry_without_lock(fs::directory_entry &entry);
  void update_preview_entries_without_lock(const int &selected);

  std::string
  format_directory_entries_without_lock(const fs::directory_entry &entry) const;

public:
  static inline std::shared_mutex file_mutex_;
  static const fs::path &current_path();
  static const fs::path &cur_parent_path();
  static const fs::path &previous_path();
  static const std::vector<fs::directory_entry> &curdir_entries();
  static const std::vector<fs::directory_entry> &preview_entries();
  static const std::vector<fs::directory_entry> &marked_entries();
  static std::vector<std::string> curdir_entries_string();
  static std::vector<std::string> preview_entries_string();
  static std::vector<std::string> marked_entries_string();
  static int previous_path_index();
  static bool yanking();
  static bool cutting();

  static void start_yanking();
  static void start_cutting();
  static void paste(const int &selected);
  static bool is_marked(const fs::directory_entry &entry);
  static void toggle_mark_on_selected(const int &selected);
  static void toggle_hidden_entries();
  static void clear_marked_entries();
  static bool delete_selected_entry(const int selected);
  static bool delete_marked_entries();
  static void update_current_path(const fs::path &new_path);
  static void update_preview_entries(const int &selected);
  static void update_curdir_entries();
  static std::string entry_name_with_icon(const fs::directory_entry &entry);
  static std::expected<fs::directory_entry, std::string>
  selected_entry(const int &selected);
  static ftxui::Element directory_preview(const int &selected);
  static std::string text_preview(const int &selected, size_t max_lines = 100,
                                  size_t max_width = 100);

  // Update preview  or current directory entries and turn entries name to
  // string
  static stdexec::sender auto load_directory_entries_async(const fs::path &path,
                                                           bool preview) {
    auto &instance = FileManager::instance();
    return stdexec::just(path, preview) |
           stdexec::then([&instance](const fs::path &path, bool preview) {
             std::unique_lock lock{FileManager::file_mutex_};
             return instance.load_directory_entries_without_lock(path, preview);
           }) |
           stdexec::then(
               [&instance](const std::vector<fs::directory_entry> &entries) {
                 if (entries.empty()) {
                   return std::vector<std::string>{"[No items]"};
                 }
                 std::vector<std::string> entries_string{};
                 entries_string.reserve(entries.size());
                 std::shared_lock lock{instance.file_mutex_};
                 for (auto &entry : entries) {
                   entries_string.push_back(
                       instance.format_directory_entries_without_lock(entry));
                 }
                 return entries_string;
               });
  }

  // When leave or enter a new directory, update current path and current
  // direcotry entries
  static stdexec::sender auto
  update_current_path_async(const fs::path &new_path) {
    std::unique_lock lock{FileManager::file_mutex_};
    auto &instance = FileManager::instance();
    instance.previous_path_ = instance.current_path_;
    instance.current_path_ = new_path;
    instance.parent_path_ = instance.current_path_.parent_path();
    return load_directory_entries_async(instance.current_path_, false);
  }
};

} // namespace duck
