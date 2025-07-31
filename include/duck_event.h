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

struct FileEvent {
  inline static std::string leave_dir = "LEAVE_DIR";
  inline static std::string enter_dir = "ENTER_DIR";
};

} // namespace duck
