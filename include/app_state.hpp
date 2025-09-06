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

  directory_to_elements(Directory directory) const {
    if (show_hidden_ && not directory.hidden_entries_.empty()) {
      std::ranges::move(directory.hidden_entries_,
                        std::back_inserter(directory.entries_));
    };
    if (directory.entries_.empty()) {
      return {ftxui::text("Empty folder")};
    }

    return directory.entries_ |
           std::views::transform([this](const fs::directory_entry &entry) {
             auto filename = ftxui::text(entry_name_with_icon(entry));
             auto marker = ftxui::text("  ");
             if (selected_entries_.contains(entry)) {
               marker = ftxui::text("█ ");
               if (is_yanking_) {
                 marker = ftxui::text("█ ") | ftxui::color(ftxui::Color::Blue);
               } else if (is_cutting_) {
                 marker = ftxui::text("█ ") | ftxui::color(ftxui::Color::Red);
               } else {
                 marker = ftxui::text("█ ");
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
  };

  std::vector<ftxui::Element> current_directory_elements() {
    return directory_to_elements(current_directory_);
  }

  std::optional<fs::directory_entry> index_entry() {
    return cache_.get(current_path_)
        .transform(
            [this](const auto &directory) -> std::vector<fs::directory_entry> {
              auto entries = directory.entries_;
              if (show_hidden_) {
                entries.reserve(entries.size() +
                                directory.hidden_entries_.size());
                std::ranges::copy(directory.hidden_entries_,
                                  std::back_inserter(entries));
                std::ranges::sort(entries, entries_sorter);
              }
              return entries;
            })
        .transform([this](const auto &entries) { return entries[index_]; });
  }
};
} // namespace duck
