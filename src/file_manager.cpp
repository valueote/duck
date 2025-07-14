#include "file_manager.h"
#include <filesystem>
namespace duck {
file_manager::file_manager()
    : selected_(0), current_path_(fs::current_path()), ui_() {}
void file_manager::run() {
  while (running_) {
    // ui_.clear_plane();
    ui_.display_current_path(current_path_);
    ui_.display_direcotry_entries(entries_, selected_);
    ui_.display_file_preview(entries_, selected_);
    ui_.render();

    int id = wait_input();
    if (id == 'q' || id == NCKEY_ESC) {
      running_ = false;
    } else if (id == NCKEY_DOWN || id == 'j') {
      if (entries_.empty())
        continue;
      move_selected_down();
    } else if (id == NCKEY_UP || id == 'k') {
      if (entries_.empty())
        continue;
      move_selected_up();
    } else if (id == NCKEY_ENTER || id == 'l') {
      if (entries_.empty())
        continue;
      if (entries_[selected_].is_directory()) {
        try {
          current_path_ = entries_[selected_].path();
          selected_ = 0;
          entries_.clear();
          for (const auto &entry : fs::directory_iterator(current_path_))
            entries_.push_back(entry);
        } catch (const fs::filesystem_error &e) {
          ui_.display_fs_error(e);
          ui_.render();
          wait_input();
        }
      }
    } else if (id == 'h') {
      if (current_path_ != current_path_.root_path()) {
        current_path_ = current_path_.parent_path();
        selected_ = 0;
        entries_.clear();
        for (const auto &entry : fs::directory_iterator(current_path_))
          entries_.push_back(entry);
      }
    } else if (id == NCKEY_RESIZE) {
      ui_.resize_plane();
    }
  }
}

int file_manager::wait_input() {
  return notcurses_get_blocking(ui_.get_nc(), &input_handler_);
}

void file_manager::move_selected_down() {
  if (selected_ + 1 < entries_.size())
    ++selected_;
}
void file_manager::move_selected_up() {
  if (selected_ > 0) {
    --selected_;
  }
}

} // namespace duck
