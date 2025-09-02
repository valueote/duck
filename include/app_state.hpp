#pragma once
#include "utils.hpp"
#include <filesystem>
#include <ftxui/dom/node.hpp>
#include <set>

namespace duck {
namespace fs = std::filesystem;

constexpr size_t lru_cache_size = 50;

struct AppState {
  fs::path current_path;
  fs::path previous_path;

  Direcotry current_direcotry;
  std::set<fs::directory_entry> selected_entries;

  bool is_yanking = false;
  bool is_cutting = false;
  bool show_hidden = false;

  int index = 0;

  // UI state
  std::string text_preview;
  ftxui::Element entries_preview;

  // Cache
  Lru<fs::path, Direcotry> cache;

  AppState() : cache(lru_cache_size) {}
};

} // namespace duck
