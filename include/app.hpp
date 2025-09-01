#pragma once
#include "app_event.hpp"
#include "event_bus.hpp"
#include "file_manager.hpp"
#include "ui.hpp"
#include <ftxui/dom/elements.hpp>
#include <thread>

namespace duck {

class App {
private:
  EventBus &event_bus_;
  Ui &ui_;
  FileManager &file_manager_;
  std::jthread event_processing_thread_;
  bool running_ = true;

  // 业务逻辑处理方法
  void handle_navigation(AppEvent event);
  void handle_file_operations(AppEvent event);
  void handle_dialog_operations(AppEvent event);
  void handle_other_operations(AppEvent event);

  // 异步任务执行
  void execute_async(std::function<void()> task);

  // 业务逻辑方法
  void update_preview_async();
  void refresh_menu_async();
  void reload_menu_async();
  void enter_directory();
  void leave_directory();
  void confirm_deletion();
  void confirm_creation();
  void confirm_rename();
  void open_file();

  // 异步发送器方法
  stdexec::sender auto update_directory_preview_async(const int &selected);
  stdexec::sender auto update_text_preview_async(const int &selected);

public:
  App(EventBus &event_bus, Ui &ui, FileManager &file_manager);

  void start_processing();

  void stop_processing();

  std::vector<ftxui::Element> entries_to_elements(
      const std::vector<std::filesystem::directory_entry> &entries);
};

} // namespace duck
