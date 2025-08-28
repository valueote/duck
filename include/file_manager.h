#pragma once
#include <exec/task.hpp>
#include <expected>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/node.hpp>
#include <list>
#include <optional>
#include <set>
#include <shared_mutex>
#include <stdexec/execution.hpp>
#include <unordered_map>
#include <vector>

namespace duck {

namespace fs = std::filesystem;

class FileManager {

private:
  template <typename Key, typename Value> class Lru {
  private:
    size_t capacity_;
    std::list<Key> lru_list_;
    std::unordered_map<Key, typename std::list<Key>::iterator> map_;
    std::unordered_map<Key, Value> cache_;
    std::shared_mutex lru_mutex_;
    void touch_without_lock(const Key &path);

  public:
    Lru(size_t capacity);
    std::optional<Value> get(const Key &path);
    void insert(const Key &path, const Value &data);
  };

  Lru<fs::path, std::vector<fs::directory_entry>> lru_cache_;
  fs::path current_path_;
  fs::path previous_path_;
  fs::path parent_path_;
  std::vector<fs::directory_entry> curdir_entries_;
  std::vector<fs::directory_entry> hidden_entries_;

  std::vector<fs::directory_entry> preview_entries_;
  std::set<fs::directory_entry> marked_entries_;
  std::vector<fs::directory_entry> clipboard_entries_;
  bool is_yanking_;
  bool is_cutting_;
  bool show_hidden_;
  static inline std::shared_mutex file_manager_mutex_;

  FileManager();

  static FileManager &instance();

  std::vector<fs::directory_entry>
  load_directory_entries_without_lock(const fs::path &path, bool show_hidden,
                                      bool use_cache);

  static bool delete_entry_without_lock(const fs::directory_entry &entry);

  static fs::path get_dest_path(const fs::directory_entry &entry,
                                const fs::path &current_path);

  static void yank_entries(const std::vector<fs::directory_entry> &entries,
                           const fs::path &current_path);
  static void rename_entries(const std::vector<fs::directory_entry> &entries,
                             const fs::path &current_path);

public:
  static const fs::path &current_path();
  static const fs::path &cur_parent_path();
  static const fs::path &previous_path();
  static const std::vector<fs::directory_entry> &curdir_entries();
  static const std::vector<fs::directory_entry> &preview_entries();
  static const std::set<fs::directory_entry> &marked_entries();
  static int previous_path_index();
  static bool yanking();
  static bool cutting();
  static std::expected<fs::directory_entry, std::string>
  selected_entry(const int &selected);

  static void start_yanking(int selected);
  static void start_cutting(int selected);
  static void yank_or_cut(int selected);
  static bool is_marked(const fs::directory_entry &entry);
  static void toggle_mark_on_selected(int selected);
  static void clear_marked_entries();
  static void toggle_hidden_entries();
  static bool delete_selected_entry(int selected);
  static void rename_selected_entry(int selected, const std::string &new_name);
  static bool delete_marked_entries();
  static void create_new_entry(const std::string &name);
  static std::vector<fs::directory_entry> update_curdir_entries(bool use_cache);
  static void update_current_path(const fs::path &new_path);

  static std::string entry_name_with_icon(const fs::directory_entry &entry);
  static std::vector<ftxui::Element>
  entries_to_elements(const std::vector<fs::directory_entry> &entries);
  static ftxui::Element
  entries_to_element(const std::vector<fs::directory_entry> &entries);
  static std::vector<fs::directory_entry>
  directory_preview(const std::pair<int, int> &selected_and_size);
  static std::string text_preview(int selected, std::pair<int, int> size);
};

} // namespace duck
