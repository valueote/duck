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
  void handle_directory_loaded(const DirecotryLoaded &event);
  void handle_preview_updated(const PreviewUpdated &event);

  void update_current_direcotry(const fs::path &path);
  void move_index_down();
  void move_index_up();
  void update_preview();
  void refresh_menu();
  void enter_directory();
  void leave_directory();
  void confirm_deletion();
  void confirm_creation();
  void confirm_rename();
  void paste_selected_entries();
  void start_yank();
  void start_cut();
  void toggle_deletion_dialog();
  void toggle_creation_dialog();
  void toggle_rename_dialog();
  void toggle_notification();
  void clear_marks();
  void open_file();

public:
  App(EventBus &event_bus, Ui &ui, FileManager &file_manager);

  void run();

  void stop();
};

} // namespace duck
