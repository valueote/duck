#pragma once
#include "event_bus.hpp"
#include <exec/async_scope.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <stdexec/execution.hpp>
#include <stdexec/stop_token.hpp>

namespace duck {
class InputHandler {
private:
  EventBus &event_bus_;
  exec::async_scope scope_;
  std::optional<stdexec::inplace_stop_source> stop_source_;
  void open_file();
  stdexec::inplace_stop_token get_token();

  stdexec::sender auto update_directory_preview_async(const int &selected);
  stdexec::sender auto update_text_preview_async(const int &selected);

public:
  InputHandler(EventBus &event_bus);
  std::function<bool(const ftxui::Event &)> navigation_handler();
  std::function<bool(ftxui::Event)> operation_handler();
  std::function<bool(const ftxui::Event &)> deletion_dialog_handler();
  std::function<bool(const ftxui::Event &)> rename_dialog_handler();
  std::function<bool(const ftxui::Event &)> creation_dialog_handler();

  std::vector<ftxui::Element>
  entries_to_elements(const std::vector<fs::directory_entry> &entries);
  void refresh_menu_async();
  void reload_menu_async();

  void update_preview_async();
  void enter_direcotry();
  void leave_direcotry();
};

} // namespace duck
