#pragma once
#include "app_state.hpp"
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

class FileManagerService {
private:
  static bool entries_sorter(const fs::directory_entry &first,
                             const fs::directory_entry &second);

  static bool delete_entry(const fs::directory_entry &entry);

  static fs::path get_dest_path(const fs::directory_entry &entry,
                                const fs::path &current_path);

  static void yank_entries(AppState &state);
  static void cut_entries(AppState &state);
  static void load_directory_entries(AppState &state, const fs::path &path,
                                     bool use_cache);

public:
  static void init(AppState &state);

  static std::vector<fs::directory_entry> curdir_entries(AppState &state);
  static std::vector<fs::directory_entry> get_entries(AppState &state,
                                                      const fs::path &target_path,
                                                      bool show_hidden);
  static int previous_path_index(AppState &state);
  static std::optional<fs::directory_entry> selected_entry(AppState &state);
  static void start_yanking(AppState &state);
  static void start_cutting(AppState &state);
  static void yank_or_cut(AppState &state);
  static bool is_marked(AppState &state, const fs::directory_entry &entry);
  static void toggle_mark_on_selected(AppState &state);
  static void clear_marked_entries(AppState &state);
  static void toggle_hidden_entries(AppState &state);
  static bool delete_selected_entry(AppState &state);
  static void rename_selected_entry(AppState &state, const std::string &new_name);
  static bool delete_marked_entries(AppState &state);
  static void create_new_entry(AppState &state, const std::string &name);
  static std::vector<fs::directory_entry> update_curdir_entries(AppState &state,
                                                                bool use_cache);
  static void update_current_path(AppState &state, const fs::path &new_path);

  static void
  directory_preview(AppState &state, const std::pair<int, int> &selected_and_size);
  static std::string text_preview(AppState &state, std::pair<int, int> size);
};

} // namespace duck