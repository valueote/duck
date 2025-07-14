#pragma once
#include <filesystem>
#include <notcurses/notcurses.h>
#include <string>
#include <vector>

namespace duck {
namespace fs = std::filesystem;
class ui {
private:
  class config {
  private:
  public:
    unsigned int rows_;
    unsigned int cols_;
    unsigned int mid_col_;
    ncplane_options left_opts_;
    ncplane_options right_opts_;

    config(ncplane *stdplane);
    void resize(ncplane *stdplane);
  };
  notcurses_options opts_;
  notcurses *nc_;
  ncplane *stdplane_;
  config config_;
  ncplane *left_plane_;
  ncplane *right_plane_;

public:
  ui();
  ~ui();
  notcurses *get_nc();
  void clear_plane();
  void resize_plane();

  void display_current_path(const std::string &path);
  void
  display_direcotry_entries(const std::vector<fs::directory_entry> &entries,
                            const size_t &selected);
  std::string format_directory_entries(const fs::directory_entry &entry);
  void display_file_preview(const std::vector<fs::directory_entry> &entries,
                            const size_t &selected);
  void display_separator();
  void render();

  void display_fs_error(const fs::filesystem_error &e);
};

} // namespace duck
