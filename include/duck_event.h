#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
namespace duck {

struct DuckEvent {
  inline static const ftxui::Event refresh{ftxui::Event::Special("")};
};

} // namespace duck
