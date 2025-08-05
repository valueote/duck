#pragma once
#include <exec/task.hpp>
#include <expected>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <shared_mutex>
#include <stdexec/execution.hpp>
#include <vector>
namespace duck {
namespace fs = std::filesystem;

class FileManager {
private:
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

  void load_directory_entries_without_lock(
      const fs::path &path, std::vector<fs::directory_entry> &entries);

  exec::task<void> lazy_load_directory_entries_without_lock(
      const fs::path &path, std::vector<fs::directory_entry> &entries,
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
  static int get_previous_path_index();
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
  get_selected_entry(const int &selected);
  static ftxui::Element get_directory_preview(const int &selected);

  static std::string get_text_preview(const int &selected,
                                      size_t max_lines = 100,
                                      size_t max_width = 100);
  // async interface
  static stdexec::sender auto
  load_directory_entries_async(const fs::path &path,
                               std::vector<fs::directory_entry> &entries) {
    auto &instance = FileManager::instance();
    return stdexec::just(path, std::ref(entries)) |
           stdexec::then(
               [&instance](const fs::path &path,
                           std::vector<fs::directory_entry> &entries) {
                 std::unique_lock lock{FileManager::file_mutex_};
                 instance.load_directory_entries_without_lock(path, entries);
                 return entries;
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

  static stdexec::sender auto
  update_current_path_async(const fs::path &new_path) {
    std::unique_lock lock{FileManager::file_mutex_};
    auto &instance = FileManager::instance();
    instance.previous_path_ = instance.current_path_;
    instance.current_path_ = new_path;
    instance.parent_path_ = instance.current_path_.parent_path();
    return load_directory_entries_async(instance.current_path_,
                                        instance.curdir_entries_);
  }
};
} // namespace duck
