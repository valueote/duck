#include "file_manager.h"
#include <filesystem>
#include <thread>
namespace duck {
file_manager::file_manager()
    : selected_(0), current_path_(fs::current_path()), ui_() {
  update_curdir_entries();
}

void file_manager::run() {
  while (running_) {
    ui_.refresh();
    ui_.display_current_path(current_path_);
    ui_.display_direcotry_entries(curdir_entries, selected_);
    ui_.display_file_preview(curdir_entries, selected_);
    ui_.render();

    int id = wait_input();
    handle_input(id);
    while ((id = get_input_nblock()) != 0) {
    }
  }
}

int file_manager::wait_input() {
  return notcurses_get_blocking(ui_.get_nc(), &input_handler_);
}

void file_manager::handle_input(int id) {
  if (id == 'q' || id == NCKEY_ESC) {
    running_ = false;
  } else if (id == NCKEY_DOWN || id == 'j') {
    if (curdir_entries.empty())
      return;
    move_selected_down();
  } else if (id == NCKEY_UP || id == 'k') {
    if (curdir_entries.empty())
      return;
    move_selected_up();
  } else if (id == NCKEY_ENTER || id == 'l') {
    if (curdir_entries.empty())
      return;
    if (curdir_entries[selected_].is_directory()) {
      try {
        current_path_ = curdir_entries[selected_].path();
        selected_ = 0;
        curdir_entries.clear();
        update_curdir_entries();
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
      curdir_entries.clear();
      update_curdir_entries();
    }
  } else if (id == NCKEY_RESIZE) {
    ui_.resize_plane();
  }
}

void file_manager::move_selected_down() {
  if (selected_ + 1 < curdir_entries.size())
    ++selected_;
}
void file_manager::move_selected_up() {
  if (selected_ > 0) {
    --selected_;
  }
}

void file_manager::update_curdir_entries() {
  for (const auto &entry : fs::directory_iterator(current_path_))
    curdir_entries.push_back(entry);
}

void file_manager::update_selected_entries() {
  for (const auto &entry : fs::directory_iterator(curdir_entries[selected_]))
    selected_dir_entries_.push_back(entry);
}

int file_manager::get_input_nblock() {
  return notcurses_get_nblock(ui_.get_nc(), &input_handler_);
}
} // namespace duck
