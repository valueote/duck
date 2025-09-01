#pragma once
#include "utils.hpp"
#include <filesystem>
#include <ftxui/dom/node.hpp>
#include <set>
#include <vector>

namespace duck {
namespace fs = std::filesystem;
struct AppState {
  fs::path current_path;
  fs::path previous_path;
  fs::path parent_path;

  std::vector<fs::directory_entry> entries;
  std::vector<fs::directory_entry> hidden_entries;
  std::set<fs::directory_entry> marked_entries;
  std::vector<fs::directory_entry> clipboard_entries;

  bool is_yanking = false;
  bool is_cutting = false;
  bool show_hidden = false;

  int selected = 0;

  // UI state
  std::string text_preview;
  ftxui::Element entries_preview;

  // Cache
  Lru<fs::path, Direcotry> cache;

  AppState() : cache(50) {}
};

} // namespace duck
