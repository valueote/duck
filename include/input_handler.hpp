#pragma once
#include "app_state.hpp"
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
  void open_file(AppState &state);
  stdexec::inplace_stop_token get_token();

public:
  InputHandler(EventBus &event_bus);
  std::function<bool(const ftxui::Event &)> navigation_handler();
  std::function<bool(ftxui::Event)> operation_handler();
  std::function<bool(const ftxui::Event &)> deletion_dialog_handler();
  std::function<bool(const ftxui::Event &)> rename_dialog_handler();
  std::function<bool(const ftxui::Event &)> creation_dialog_handler();
};

} // namespace duck
