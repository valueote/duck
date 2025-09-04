#pragma once
#include "app_event.hpp"
#include "app_state.hpp"
#include "event_bus.hpp"
#include "ui.hpp"
#include <ftxui/dom/elements.hpp>
#include <thread>

namespace duck {

class FileManager; // Forward declaration

class App {
private:
  AppState state_;
  EventBus &event_bus_;
  Ui &ui_;
  FileManager &file_manager_;
  std::jthread event_processing_thread_;
  bool running_ = false;

  void process_events();
  void handle_fmgr_event(const FmgrEvent &event);
  void handle_render_event(const RenderEvent &event);
  void handle_directory_loaded(const DirectoryLoaded &event);

  void move_index_down();
  void move_index_up();
  void update_preview();
  void refresh_menu();
  void reload_menu();
  void enter_directory();
  void leave_directory();
  void confirm_deletion();
  void confirm_creation();
  void confirm_rename();
  void open_file();

public:
  App(EventBus &event_bus, Ui &ui, FileManager &file_manager);

  void run();

  void stop();
};

} // namespace duck
