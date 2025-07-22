#pragma once
#include <filesystem>
#include <vector>
namespace duck {
namespace fs = std::filesystem;

class FileManager {
private:
  fs::path current_path_;
  fs::path previous_path_;
  fs::path parent_path_;
  std::vector<fs::directory_entry> curdir_entries_;
  std::vector<fs::directory_entry> preview_entries_;
  std::vector<fs::directory_entry> selected_entires_;

  bool is_yangking_;
  bool is_cutting_;

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
  const std::vector<fs::directory_entry> &selected_entries() const;
  std::vector<std::string> curdir_entries_string() const;
  std::vector<std::string> preview_entries_string() const;
  std::vector<std::string> selected_entries_string() const;
  int get_previous_path_index() const;
  bool yangking() const;
  bool cutting() const;

  void toggle_selected(const int &selected);
  void toggle_yangking();
  void toggle_cutting();
  bool is_selected(const fs::directory_entry &entry) const;
  void clear_selected_entries();
  bool delete_selected_entry(const int selected);
  bool delete_selected_entries();
  void update_current_path(const fs::path &new_path);
  void update_preview_entries(const int &selected);
  void update_curdir_entries();

  const std::optional<fs::directory_entry>
  get_selected_entry(const int &selected) const;
  const std::string
  format_directory_entries(const fs::directory_entry &entry) const;
};
} // namespace duck
