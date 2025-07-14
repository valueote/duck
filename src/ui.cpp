#include "ui.h"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <notcurses/notcurses.h>
#include <string>
#include <unordered_map>
namespace duck {
std::string get_text_preview(const fs::path &path, size_t max_lines = 20,
                             size_t max_width = 80) {
  if (!fs::is_regular_file(path))
    return "[Not a regular file]";
  std::ifstream file(path);
  if (!file)
    return "[Failed to open file]";

  std::ostringstream oss;
  std::string line;
  size_t lines = 0;

  while (std::getline(file, line) && lines < max_lines) {

    if (line.length() > max_width) {
      line = line.substr(0, max_width - 3) + "...";
    }

    for (auto &c : line) {
      if (iscntrl(static_cast<unsigned char>(c))) {
        c = '?';
      }
    }

    oss << line << '\n';
    ++lines;
  }
  return oss.str();
}

ui::config::config(ncplane *stdplane) {
  ncplane_dim_yx(stdplane, &rows_, &cols_);
  mid_col_ = cols_ / 2;
  left_opts_ = {
      .y = 0,
      .x = 0,
      .rows = rows_,
      .cols = mid_col_,
      .userptr = nullptr,
      .name = "left",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0,
      .margin_r = 0,
  };
  right_opts_ = {
      .y = 0,
      .x = static_cast<int>(mid_col_) + 1,
      .rows = rows_,
      .cols = cols_ - mid_col_ - 1,
      .userptr = nullptr,
      .name = "right",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0,
      .margin_r = 0,
  };
}

void ui::config::resize(ncplane *stdplane) {
  ncplane_dim_yx(stdplane, &rows_, &cols_);
  mid_col_ = cols_ / 2;
}

ui::ui()
    : opts_{.flags = 0}, nc_{notcurses_init(&opts_, nullptr)},
      stdplane_{notcurses_stdplane(nc_)}, config_{stdplane_},
      left_plane_{ncplane_create(stdplane_, &config_.left_opts_)},
      right_plane_{ncplane_create(stdplane_, &config_.right_opts_)} {}

ui::~ui() { notcurses_stop(nc_); }

void ui::resize_plane() {
  config_.resize(stdplane_);

  ncplane_erase(stdplane_);

  unsigned int new_left_cols = config_.mid_col_;
  unsigned int new_right_x = config_.mid_col_ + 1;
  unsigned int new_right_cols = config_.cols_ - config_.mid_col_ - 1;

  ncplane_resize_simple(left_plane_, config_.rows_, new_left_cols);

  ncplane_move_yx(right_plane_, 0, new_right_x);
  ncplane_resize_simple(right_plane_, config_.rows_, new_right_cols);

  display_separator();
}

void ui::clear_plane() {
  ncplane_destroy(left_plane_);
  ncplane_destroy(right_plane_);
}

void ui::display_current_path(const std::string &path) {
  ncplane_erase(left_plane_);
  ncplane_printf_yx(left_plane_, 0, 0, "\uf4d3 %s", path.c_str());
}

void ui::display_direcotry_entries(
    const std::vector<fs::directory_entry> &entries, const size_t &selected) {
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i + 2 >= config_.rows_)
      break;
    std::string name = format_directory_entries(entries[i]);
    if (i == selected)
      ncplane_printf_yx(left_plane_, i + 2, 0, "> %s", name.c_str());
    else
      ncplane_printf_yx(left_plane_, i + 2, 2, "%s", name.c_str());
  }
}

void ui::display_file_preview(const std::vector<fs::directory_entry> &entries,
                              const size_t &selected) {

  ncplane_erase(right_plane_);
  if (!entries.empty() && selected < entries.size()) {
    const auto &entry = entries[selected];
    if (entry.is_regular_file()) {
      std::string preview = get_text_preview(entry.path(), config_.rows_ - 3,
                                             ncplane_dim_x(right_plane_) - 2);
      ncplane_printf_yx(right_plane_, 0, 0, "\uf15c Preview : %s ",
                        entry.path().filename().c_str());
      int y = 2;
      std::istringstream ss(preview);
      std::string line;
      while (std::getline(ss, line) &&
             y < static_cast<int>(config_.rows_) - 1) {
        ncplane_putstr_yx(right_plane_, y++, 0, line.c_str());
      }
    } else if (entry.is_directory()) {
      ncplane_printf_yx(right_plane_, 0, 0, "\uf4d3 Directory: %s",
                        entry.path().filename().c_str());
    } else {
      ncplane_printf_yx(right_plane_, 0, 0, "[Unsupported file type]");
    }
  } else if (entries.empty()) {
    ncplane_printf_yx(right_plane_, 0, 0, "\uf4d3 Empty directory");
  }
}
void ui::display_separator() {
  const char *separator_char = "│";

  for (int y = 0; y < config_.rows_; ++y) {
    ncplane_putstr_yx(stdplane_, y, config_.mid_col_, separator_char);
  }
}

notcurses *ui::get_nc() { return nc_; }

void ui::render() { notcurses_render(nc_); }

void ui::display_fs_error(const fs::filesystem_error &e) {
  ncplane_printf_yx(right_plane_, 0, 0, "Error: %s", e.what());
}

std::string ui::format_directory_entries(const fs::directory_entry &entry) {
  static const std::unordered_map<std::string, std::string> extension_icons{
      {".txt", "\uf15c"}, {".md", "\ueeab"},   {".cpp", "\ue61d"},
      {".hpp", "\uf0fd"}, {".h", "\uf0fd"},    {".c", "\ue61e"},
      {".jpg", "\uf4e5"}, {".jpeg", "\uf4e5"}, {".png", "\uf4e5"},
      {".gif", "\ue60d"}, {".pdf", "\ue67d"},  {".zip", "\ue6aa"},
      {".mp3", "\uf001"}, {".mp4", "\uf03d"},  {".json", "\ue60b"},
      {".log", "\uf4ed"}, {".csv", "\ueefc"},
  };

  const auto filename = entry.path().filename().string();

  if (fs::is_directory(entry)) {
    return std::format("\uf4d3 {}", filename);
  }

  auto ext = entry.path().extension().string();
  std::ranges::transform(ext, ext.begin(), [](char c) {
    return static_cast<char>(std::tolower(c));
  });

  auto icon_it = extension_icons.find(ext);
  const std::string &icon =
      icon_it != extension_icons.end() ? icon_it->second : "\uf15c";

  return std::format("{} {}", icon, filename);
}

} // namespace duck
