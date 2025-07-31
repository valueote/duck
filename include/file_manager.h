#pragma once
#include "scheduler.h"
#include <expected>
#include <filesystem>
#include <print>
#include <stdexec/execution.hpp>
#include <vector>
namespace duck {
namespace fs = std::filesystem;

class FileManager {
private:
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

  void load_directory_entries(const fs::path &path,
                              std::vector<fs::directory_entry> &entries);
  bool delete_entry(fs::directory_entry &entry);

public:
  FileManager();

  const fs::path &current_path() const;
  const fs::path &cur_parent_path() const;
  const fs::path &previous_path() const;
  const std::vector<fs::directory_entry> &curdir_entries() const;
  const std::vector<fs::directory_entry> &preview_entries() const;
  const std::vector<fs::directory_entry> &marked_entries() const;
  std::vector<std::string> curdir_entries_string() const;
  std::vector<std::string> preview_entries_string() const;
  std::vector<std::string> marked_entries_string() const;
  int get_previous_path_index() const;
  bool yanking() const;
  bool cutting() const;

  void toggle_mark_on_selected(const int &selected);
  void start_yanking();
  void start_cutting();
  void paste(const int &selected);
  bool is_marked(const fs::directory_entry &entry) const;
  void clear_marked_entries();
  bool delete_selected_entry(const int selected);
  bool delete_marked_entries();
  void update_current_path(const fs::path &new_path);
  void update_preview_entries(const int &selected);
  void update_curdir_entries();
  void toggle_hidden_entries();

  std::expected<fs::directory_entry, std::string>
  get_selected_entry(const int &selected) const;
  std::string entry_name_with_icon(const fs::directory_entry &entry) const;
  std::string format_directory_entries(const fs::directory_entry &entry) const;

  stdexec::sender auto
  load_directory_entries_async(const fs::path &path,
                               std::vector<fs::directory_entry> &entries) {
    return stdexec::just(path, std::ref(entries)) |
           stdexec::then([this](const fs::path &path,
                                std::vector<fs::directory_entry> &entries) {
             if (!fs::is_directory(path)) {
               return std::vector<fs::directory_entry>{};
             }
             entries.clear();
             try {
               std::vector<fs::directory_entry> dirs;
               std::vector<fs::directory_entry> files;

               dirs.reserve(128);
               files.reserve(128);

               for (const auto &entry : fs::directory_iterator(path)) {
                 if (entry.path().filename().string().starts_with('.') &&
                     !show_hidden_) {
                   continue;
                 }
                 (entry.is_directory() ? dirs : files).push_back(entry);
               }

               entries.reserve(dirs.size() + files.size());
               std::ranges::sort(dirs);
               std::ranges::sort(files);
               std::ranges::copy(dirs, std::back_inserter(entries));
               std::ranges::copy(files, std::back_inserter(entries));

             } catch (const std::exception &e) {
               std::println(stderr, "[ERROR]: {} in load_directory_entries",
                            e.what());
             }
             return entries;
           }) |
           stdexec::then(
               [this](const std::vector<fs::directory_entry> entries) {
                 return entries |
                        std::views::transform(
                            [this](const fs::directory_entry &entry) {
                              return format_directory_entries(entry);
                            }) |
                        std::ranges::to<std::vector>();
               });
  }

  stdexec::sender auto update_curdir_entries_async() {
    return stdexec::on(
        Scheduler::io_scheduler(),
        load_directory_entries_async(current_path_, curdir_entries_));
  }

  stdexec::sender auto update_current_path_async(const fs::path &new_path) {
    previous_path_ = current_path_;
    current_path_ = new_path;
    parent_path_ = current_path_.parent_path();
    return update_curdir_entries_async();
  }
};
} // namespace duck
