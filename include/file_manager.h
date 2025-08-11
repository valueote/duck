#pragma once
#include <exec/task.hpp>
#include <expected>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/node.hpp>
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
  private:
    size_t capacity_;
    std::list<fs::path> lru_list_;
    std::unordered_map<fs::path, std::list<fs::path>::iterator> map_;
    std::unordered_map<fs::path, std::vector<fs::directory_entry>> cache_;
    std::shared_mutex lru_mutex_;
    void touch_without_lock(const fs::path &path);

  public:
    Lru(size_t capacity);
    std::optional<std::vector<fs::directory_entry>> get(const fs::path &path);
    void insert(const fs::path &path,
                const std::vector<fs::directory_entry> &data);
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
  load_directory_entries_without_lock(const fs::path &path, bool show_hidden);
  bool delete_entry_without_lock(fs::directory_entry &entry);

  std::string
  format_directory_entries_without_lock(const fs::directory_entry &entry) const;

public:
  static inline std::shared_mutex file_manager_mutex_;
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

  static std::vector<std::string>
  format_entries(const std::vector<fs::directory_entry> &entries);
  static ftxui::Element
  entries_string_to_element(std::vector<std::string> entries);

  static std::expected<fs::directory_entry, std::string>
  selected_entry(const int &selected);

  static std::string entry_name_with_icon(const fs::directory_entry &entry);
  static std::string text_preview(const int &selected, size_t max_lines = 100,
                                  size_t max_width = 100);

  static stdexec::sender auto update_curdir_entries();
  static stdexec::sender auto update_current_path(const fs::path &new_path);
  static stdexec::sender auto directory_preview(const int &selected);
  static stdexec::sender auto text_preview_async(const int &selected);
};

inline stdexec::sender auto FileManager::update_curdir_entries() {
  auto &instance = FileManager::instance();
  return stdexec::just() | stdexec::then([]() {
           fs::path target_path{};
           bool show_hidden{};
           auto &instance = FileManager::instance();

           {
             std::shared_lock lock{file_manager_mutex_};
             target_path = instance.current_path_;
             show_hidden = instance.show_hidden_;
           }

           auto entries =
               std::move(instance.load_directory_entries_without_lock(
                   target_path, show_hidden));

           {
             std::unique_lock lock{file_manager_mutex_};
             instance.curdir_entries_ = std::move(entries);
             return instance.curdir_entries_;
           }
         });
}

inline stdexec::sender auto
FileManager::update_current_path(const fs::path &new_path) {
  {
    std::unique_lock lock{FileManager::file_manager_mutex_};
    auto &instance = FileManager::instance();
    instance.previous_path_ = instance.current_path_;
    instance.current_path_ = new_path;
    instance.parent_path_ = instance.current_path_.parent_path();
  }

  return update_curdir_entries();
}

inline stdexec::sender auto
FileManager::directory_preview(const int &selected) {
  return stdexec::just(selected) | stdexec::then([](const int &selected) {
           fs::path target_path;
           bool show_hidden{};

           {
             auto &instance = FileManager::instance();
             std::shared_lock lock{file_manager_mutex_};
             if (instance.curdir_entries_.empty()) {
               return;
             }
             if (selected < 0 || selected >= instance.curdir_entries_.size()) {
               return;
             }
             if (not fs::is_directory(instance.curdir_entries_[selected])) {
               return;
             }
             target_path = instance.curdir_entries_[selected].path();
             show_hidden = instance.show_hidden_;
           }

           auto entries =
               std::move(instance().load_directory_entries_without_lock(
                   target_path, show_hidden));
           {
             std::unique_lock lock{file_manager_mutex_};
             instance().preview_entries_ = std::move(entries);
           }
         }) |
         stdexec::then([]() {
           std::shared_lock lock{file_manager_mutex_};
           return instance().preview_entries_;
         });
}

} // namespace duck
