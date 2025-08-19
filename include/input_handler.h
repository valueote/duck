#pragma once
#include "file_manager.h"
#include "scheduler.h"
#include "ui.h"
#include <exec/async_scope.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <stdexec/execution.hpp>
#include <vector>

namespace duck {
class InputHandler {
private:
  Ui &ui_;
  exec::async_scope scope_;
  void open_file();

public:
  InputHandler(Ui &ui);
  std::function<bool(const ftxui::Event &)> navigation_handler();
  std::function<bool(ftxui::Event)> test_handler();
  std::function<bool(const ftxui::Event &)> deletion_dialog_handler();
  std::function<bool(const ftxui::Event &)> rename_dialog_handler();
  void update_preview_async();
  void enter_direcotry();
  void leave_direcotry();

  stdexec::sender auto update_directory_preview_async(const int &selected);
  stdexec::sender auto update_text_preview_async(const int &selected);
  void refresh_menu_async();
};

inline void InputHandler::refresh_menu_async() {
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then(FileManager::curdir_entries) |
              stdexec::then(FileManager::entries_to_elements) |
              stdexec::then([this](std::vector<ftxui::Element> element) {
                ui_.post_task([this, elmt = std::move(element)]() {
                  ui_.update_curdir_entries(std::move(elmt));
                  ui_.post_event(ftxui::Event::Custom);
                });
              });
  scope_.spawn(task);
}

} // namespace duck
