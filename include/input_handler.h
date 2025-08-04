#pragma once
#include "file_manager.h"
#include "scheduler.h"
#include "ui.h"
#include <ftxui/component/event.hpp>
#include <stdexec/execution.hpp>

namespace duck {
class InputHandler {
private:
  Ui &ui_;

  void open_file();

public:
  InputHandler(Ui &ui);
  std::function<bool(ftxui::Event)> navigation_handler();
  std::function<bool(ftxui::Event)> test_handler();
  std::function<bool(ftxui::Event)> deletetion_dialog_handler();

  stdexec::sender auto
  update_directory_preview_async(const fs::path &dir_path) {
    return stdexec::schedule(Scheduler::io_scheduler()) |
           stdexec::then([this, dir_path]() {
             return FileManager::get_directory_preview(ui_.selected(),
                                                       std::move(dir_path));
           }) |
           stdexec::then([this](ftxui::Element preview) {
             ui_.post_task([this, preview]() {
               ui_.update_entries_preview(std::move(preview));
             });
           });

    ;
  }

  stdexec::sender auto update_text_preview_async(const fs::path &dir_path) {
    return stdexec::schedule(Scheduler::io_scheduler()) |
           stdexec::then([this, dir_path]() {
             return FileManager::get_text_preview(dir_path);
           }) |
           stdexec::then([this](std::string preview) {
             ui_.post_task([this, preview]() {
               ui_.update_text_preview(std::move(preview));
             });
           });
  }

  void update_preview_async();
  void enter_direcotry();
  void leave_direcotry();
};

} // namespace duck
