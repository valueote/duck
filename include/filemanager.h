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
  std::vector<fs::directory_entry> marked_entires_;
  std::vector<fs::directory_entry> clipboard_entries_;

  bool is_yanking_;
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

  std::optional<fs::directory_entry>
  get_selected_entry(const int &selected) const;
  std::string entry_name_with_icon(const fs::directory_entry &entry) const;
  std::string format_directory_entries(const fs::directory_entry &entry) const;
};
} // namespace duck
