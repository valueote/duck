#pragma once
#include "app_event.hpp"
#include "colorscheme.hpp"
#include "utils.hpp"
#include <algorithm>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <iterator>
#include <ranges>
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

  AppState() : cache_(lru_cache_size) {}

  std::vector<ftxui::Element>
  entries_to_elements(const std::vector<fs::directory_entry> &entries) const {
    return entries |
           std::views::transform([this](const fs::directory_entry &entry) {
             auto filename = ftxui::text(entry_name_with_icon(entry));
             auto marker = ftxui::text("  ");
             if (selected_entries_.contains(entry)) {
               marker = ftxui::text("█ ");
               if (is_yanking_) {
                 marker = marker | ftxui::color(ftxui::Color::Blue);
               } else if (is_cutting_) {
                 marker = marker | ftxui::color(ftxui::Color::Red);
               }
             }
             auto elmt = ftxui::hbox({marker, filename});
             if (entry.is_directory()) {
               elmt |= ftxui::color(ColorScheme::dir());
             } else {
               elmt |= ftxui::color(ColorScheme::file());
             }
             return elmt;
           }) |
           std::ranges::to<std::vector>();
  }

  std::vector<ftxui::Element> current_directory_elements() {
    auto entries = get_entries(current_path_);
    if (entries.empty()) {
      return {ftxui::text("[Empty folder]")};
    }

    return entries_to_elements(entries);
  }

  size_t get_entries_size(const fs::path &path) {
    auto directory_opt = cache_.get(path);
    if (!directory_opt) {
      return 0;
    }
    const auto &directory = directory_opt.value();
    if (show_hidden_) {
      return directory.entries_.size() + directory.hidden_entries_.size();
    }
    return directory.entries_.size();
  }

  std::vector<fs::directory_entry> get_entries(const fs::path &path) {
    auto directory_opt = cache_.get(path);
    if (!directory_opt) {
      return {};
    }
    auto directory = directory_opt.value();
    auto entries = directory.entries_;
    if (show_hidden_) {
      entries.reserve(entries.size() + directory.hidden_entries_.size());
      std::ranges::copy(directory.hidden_entries_, std::back_inserter(entries));
      std::ranges::sort(entries, entries_sorter);
    }
    return entries;
  }

  std::optional<fs::directory_entry> indexed_entry() {
    auto entries = get_entries(current_path_);
    if (index_ < entries.size()) {
      return entries[index_];
    }
    return std::nullopt;
  }

  void toggle_hidden() {
    auto entry_opt = indexed_entry();

    show_hidden_ = !show_hidden_;

    if (entry_opt.has_value()) {
      const auto &current_entry = entry_opt.value();
      auto entries = get_entries(current_path_);
      auto it = std::ranges::find(
          entries, current_entry.path(),
          [](const fs::directory_entry &e) { return e.path(); });

      if (it != entries.end()) {
        index_ = std::distance(entries.begin(), it);
      } else {
        index_ = 0;
      }
    } else {
      index_ = 0;
    }
  }

  void remove_entries(const std::vector<fs::path> &paths) {
    for (const auto &path : paths) {
      if (auto directory = cache_.get(path.parent_path());
          directory.has_value()) {
        auto pred = [path](const auto &entry) { return entry.path() == path; };
        std::erase_if(directory.value().entries_, pred);
        std::erase_if(directory.value().hidden_entries_, pred);
        cache_.insert(std::move(path.parent_path()),
                      std::move(directory.value()));
      }
    }
  }
};
} // namespace duck
