#include "input_handler.hpp"
#include "app_event.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>
#include <functional>
#include <sys/wait.h>
#include <utility>

namespace duck {
using std::exit;
using std::move;

InputHandler::InputHandler(EventBus &event_bus) : event_bus_{event_bus} {}

std::function<bool(const ftxui::Event &)> InputHandler::navigation_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Character('j')) {
      event_bus_.push_event(RenderEvent{RenderEvent::Type::MoveSelectionDown});
      return true;
    }

    if (event == ftxui::Event::Character('k')) {
      event_bus_.push_event(RenderEvent{RenderEvent::Type::MoveSelectionUp});
      return true;
    }

    if (event == ftxui::Event::Character('l')) {
      event_bus_.push_event(RenderEvent{RenderEvent::Type::EnterDirectory});
      return true;
    }

    if (event == ftxui::Event::Character('h')) {
      event_bus_.push_event(RenderEvent{RenderEvent::Type::LeaveDirectory});
      return true;
    }

    if (event == ftxui::Event::Character('q')) {
      event_bus_.push_event(RenderEvent{RenderEvent::Type::Quit});
      return true;
    }

    if (event == ftxui::Event::Escape) {
      event_bus_.push_event(RenderEvent{RenderEvent::Type::ClearMarks});
      return true;
    }

    return false;
  };
}

std::function<bool(ftxui::Event)> InputHandler::operation_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Return) {
      event_bus_.push_event(FmgrEvent{FmgrEvent::Type::OpenFile});
      return true;
    }

    if (event == ftxui::Event::Character(' ')) {
      event_bus_.push_event(FmgrEvent{FmgrEvent::Type::ToggleMark});
      return true;
    }

    if (event == ftxui::Event::Character('d')) {
      event_bus_.push_event(
          RenderEvent{RenderEvent::Type::ToggleDeletionDialog});
      return true;
    }

    if (event == ftxui::Event::Character('a')) {
      event_bus_.push_event(
          RenderEvent{RenderEvent::Type::ToggleCreationDialog});
      return true;
    }

    if (event == ftxui::Event::Character('r')) {
      event_bus_.push_event(RenderEvent{RenderEvent::Type::ToggleRenameDialog});
      return true;
    }

    if (event == ftxui::Event::Character('p')) {
      event_bus_.push_event(FmgrEvent{FmgrEvent::Type::Paste});
      return true;
    }

    if (event == ftxui::Event::Character('.')) {
      event_bus_.push_event(FmgrEvent{FmgrEvent::Type::ToggleHidden});
      return true;
    }

    if (event == ftxui::Event::Character('n')) {
      event_bus_.push_event(RenderEvent{RenderEvent::Type::ToggleNotification});
      return true;
    }

    return false;
  };
}

std::function<bool(const ftxui::Event &)>
InputHandler::deletion_dialog_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Character('y')) {
      event_bus_.push_event(FmgrEvent{FmgrEvent::Type::Deletion});
      return true;
    }

    if (event == ftxui::Event::Character('n') ||
        event == ftxui::Event::Escape) {
      event_bus_.push_event(
          RenderEvent{RenderEvent::Type::ToggleDeletionDialog});
      return true;
    }

    return false;
  };
}

std::function<bool(const ftxui::Event &)>
InputHandler::rename_dialog_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Escape) {
      event_bus_.push_event(
          RenderEvent{RenderEvent::Type::ToggleDeletionDialog});
      return true;
    }

    if (event == ftxui::Event::Return) {
      event_bus_.push_event(FmgrEvent{FmgrEvent::Type::Rename});
      return true;
    }
    return false;
  };
}

std::function<bool(const ftxui::Event &)>
InputHandler::creation_dialog_handler() {
  return [this](const ftxui::Event &event) {
    if (event == ftxui::Event::Escape) {
      event_bus_.push_event(
          RenderEvent{RenderEvent::Type::ToggleCreationDialog});
      return true;
    }

    if (event == ftxui::Event::Return) {
      event_bus_.push_event(FmgrEvent{FmgrEvent::Type::Creation});
      return true;
    }
    return false;
  };
}

} // namespace duck
