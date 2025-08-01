#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <vector>
namespace duck {

struct DirectoryChangedEvent : public ftxui::Event {
  std::vector<std::string> entries;
  int previous_path_index;
  DirectoryChangedEvent(std::vector<std::string> e, int idx)
      : entries(std::move(e)), previous_path_index(idx) {}
};

struct DuckEvent {
  inline static const ftxui::Event leave_dir{
      ftxui::Event::Special("LEAVE_DIR")};
  inline static const ftxui::Event enter_dir{
      ftxui::Event::Special("ENTER_DIR")};
  inline static const ftxui::Event refresh{ftxui::Event::Special("")};
};

} // namespace duck
