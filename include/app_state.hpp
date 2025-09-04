#pragma once
#include "app_event.hpp"
#include "colorscheme.hpp"
#include "utils.hpp"
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ranges>
#include <set>
#include <variant>

namespace duck {
namespace fs = std::filesystem;

constexpr size_t lru_cache_size = 50;

struct AppState {
  fs::path current_path_;
  fs::path previous_path_;

  Direcotry current_direcotry_;
  std::set<fs::directory_entry> selected_entries_;

  bool is_yanking_ = false;
  bool is_cutting_ = false;
  bool show_hidden_ = false;

  size_t index = 0;

  // UI state

  // Cache
  Lru<fs::path, Direcotry> cache;

  AppState() : cache(lru_cache_size) {}

  std::vector<ftxui::Element> current_direcotry_elements() const {
    if (current_direcotry_.entries_.empty()) {
      return {};
    }
    return current_direcotry_.entries_ |
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
  }

  EntryPreview current_preview() const {
    if (current_direcotry_.empty()) {
      return std::monostate();
    }
  }
};

} // namespace duck
