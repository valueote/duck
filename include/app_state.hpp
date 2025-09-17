#pragma once
#include "utils.hpp"
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <optional>
#include <set>
#include <vector>

namespace duck {
namespace fs = std::filesystem;

constexpr size_t lru_cache_size = 50;

struct AppState {
  fs::path current_path_;
  fs::path previous_path_;

  Directory current_directory_;
  std::set<fs::directory_entry> selected_entries_;

  bool is_yanking_ = false;
  bool is_cutting_ = false;
  bool show_hidden_ = false;

  size_t index_ = 0;

  // UI state

  // Cache
  Lru<fs::path, Directory> cache_;

  AppState();

  std::vector<ftxui::Element>
  entries_to_elements(const std::vector<fs::directory_entry> &entries) const;
  std::vector<ftxui::Element> current_directory_elements();
  std::vector<ftxui::Element> selected_entries_elements();
  std::vector<ftxui::Element> indexed_entry_elements();
  size_t entries_size(const fs::path &path);
  std::optional<std::vector<fs::directory_entry>>
  get_entries(const fs::path &path);
  std::vector<fs::path> selected_entries_paths();
  std::optional<fs::directory_entry> indexed_entry();
  void move_index_down();
  void move_index_up();
  void toggle_hidden();
  void remove_entries(const std::vector<fs::path> &paths);
  void rename_entry(const fs::path &old_name, const fs::path &new_name);
  void create_entry(const fs::path &new_entry);
};
} // namespace duck
