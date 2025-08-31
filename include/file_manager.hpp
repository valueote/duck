#pragma once
#include "utils.hpp"
#include <exec/task.hpp>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/node.hpp>
#include <set>
#include <shared_mutex>
#include <stdexec/execution.hpp>
#include <vector>

namespace duck {

namespace fs = std::filesystem;

class FileManager {

private:
  Lru<fs::path, Direcotry> cache_;
  fs::path current_path_;
  fs::path previous_path_;
  fs::path parent_path_;

  std::vector<fs::directory_entry> preview_entries_;
  std::set<fs::directory_entry> marked_entries_;
  std::vector<fs::directory_entry> clipboard_entries_;
  bool is_yanking_;
  bool is_cutting_;
  bool show_hidden_;
  std::shared_mutex file_manager_mutex_;

  static bool entries_sorter(const fs::directory_entry &first,
                             const fs::directory_entry &second);

  static bool delete_entry(const fs::directory_entry &entry);

  static fs::path get_dest_path(const fs::directory_entry &entry,
                                const fs::path &current_path);

  static void yank_entries(const std::vector<fs::directory_entry> &entries,
                           const fs::path &current_path);
  static void cut_entries(const std::vector<fs::directory_entry> &entries,
                          const fs::path &current_path);
  void load_directory_entries(const fs::path &path, bool use_cache);

public:
  FileManager();

  const fs::path &current_path();
  const fs::path &cur_parent_path();
  const fs::path &previous_path();
  std::vector<fs::directory_entry> curdir_entries();
  std::vector<fs::directory_entry> get_entries(const fs::path &target_path,
                                               bool show_hidden);
  std::vector<fs::directory_entry> preview_entries();
  std::set<fs::directory_entry> marked_entries();
  int previous_path_index();
  bool yanking();
  bool cutting();
  std::optional<fs::directory_entry> selected_entry(const int &selected);
  void start_yanking(int selected);
  void start_cutting(int selected);
  void yank_or_cut(int selected);
  bool is_marked(const fs::directory_entry &entry);
  void toggle_mark_on_selected(int selected);
  void clear_marked_entries();
  void toggle_hidden_entries();
  bool delete_selected_entry(int selected);
  void rename_selected_entry(int selected, const std::string &new_name);
  bool delete_marked_entries();
  void create_new_entry(const std::string &name);
  std::vector<fs::directory_entry> update_curdir_entries(bool use_cache);
  void update_current_path(const fs::path &new_path);

  std::vector<fs::directory_entry>
  directory_preview(const std::pair<int, int> &selected_and_size);
  std::string text_preview(int selected, std::pair<int, int> size);
};

} // namespace duck
