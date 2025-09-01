#pragma once
#include "app_event.hpp"
#include "app_state.hpp"
#include "event_bus.hpp"
#include "ui.hpp"
#include <ftxui/dom/elements.hpp>
#include <thread>

namespace duck {

class App {
private:
  AppState state_;
  EventBus &event_bus_;
  Ui &ui_;
  std::jthread event_processing_thread_;
  bool running_ = true;

  void process_events();
  void handle_fmgr_event(const FmgrEvent &event);
  void handle_render_event(const RenderEvent &event);

  void update_preview_async();
  void refresh_menu_async();
  void reload_menu_async();
  void enter_directory();
  void leave_directory();
  void confirm_deletion();
  void confirm_creation();
  void confirm_rename();
  void open_file();

  stdexec::sender auto update_directory_preview_async();
  stdexec::sender auto update_text_preview_async();

public:
  App(EventBus &event_bus, Ui &ui);

  void run();

  void stop();

  std::vector<ftxui::Element> entries_to_elements(
      const std::vector<std::filesystem::directory_entry> &entries);
};

} // namespace duck
