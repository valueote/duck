#include "ui.h"
#include <filesystem>
#include <fstream>
#include <notcurses/notcurses.h>
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
    // 截断过长的行
    if (line.length() > max_width) {
      line = line.substr(0, max_width - 3) + "...";
    }

    // 替换控制字符
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
      .x = static_cast<int>(mid_col_),
      .rows = rows_,
      .cols = cols_ - mid_col_,
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
  left_opts_.cols = mid_col_;
  right_opts_.x = static_cast<int>(mid_col_);
  right_opts_.cols = cols_ - mid_col_;
}

ui::ui()
    : opts_{.flags = 0}, nc_{notcurses_init(&opts_, nullptr)},
      stdplane_{notcurses_stdplane(nc_)}, config_{stdplane_},
      left_plane_{ncplane_create(stdplane_, &config_.left_opts_)},
      right_plane_{ncplane_create(stdplane_, &config_.right_opts_)} {}

ui::~ui() { notcurses_stop(nc_); }

void ui::resize_plane() {
  config_.resize(stdplane_);
  clear_plane();
  left_plane_ = ncplane_create(stdplane_, &config_.left_opts_);
  right_plane_ = ncplane_create(stdplane_, &config_.right_opts_);
}

void ui::clear_plane() {
  ncplane_destroy(left_plane_);
  ncplane_destroy(right_plane_);
}

void ui::display_current_path(const std::string &path) {
  ncplane_printf_yx(left_plane_, 0, 0, "📁 %s", path.c_str());
}

void ui::display_direcotry_entries(
    const std::vector<fs::directory_entry> &entries, size_t selected) {
  for (size_t i = 0; i < entries.size(); ++i) {
    std::string name = entries[i].path().filename().string();
    if (i == selected)
      ncplane_printf_yx(left_plane_, i + 2, 0, "> %s", name.c_str());
    else
      ncplane_printf_yx(left_plane_, i + 2, 2, "%s", name.c_str());
  }
}
void ui::display_file_preview(const std::vector<fs::directory_entry> &entries,
                              size_t selected) {
  if (!entries.empty() && selected >= 0 &&
      static_cast<size_t>(selected) < entries.size()) {
    const auto &entry = entries[selected];
    if (entry.is_regular_file()) {
      std::string preview =
          get_text_preview(entry.path(), config_.rows_ - 3, config_.cols_ - 10);
      ncplane_printf_yx(right_plane_, 0, 0, "📄 Preview: %s",
                        entry.path().filename().c_str());
      int y = 2;
      std::istringstream ss(preview);
      std::string line;
      while (std::getline(ss, line) &&
             y < static_cast<int>(config_.rows_) - 1) {
        // 使用 ncplane_putstr_yx 处理特殊字符
        ncplane_putstr_yx(right_plane_, y++, 0, line.c_str());
      }
    } else if (entry.is_directory()) {
      ncplane_printf_yx(right_plane_, 0, 0, "📁 Directory: %s",
                        entry.path().filename().c_str());
    } else {
      ncplane_printf_yx(right_plane_, 0, 0, "[Unsupported file type]");
    }
  } else if (entries.empty()) {
    ncplane_printf_yx(right_plane_, 0, 0, "📁 Empty directory");
  }
}
notcurses *ui::get_nc() { return nc_; }

void ui::render() { notcurses_render(nc_); }

void ui::display_fs_error(const fs::filesystem_error &e) {
  ncplane_printf_yx(right_plane_, 0, 0, "Error: %s", e.what());
}

} // namespace duck
