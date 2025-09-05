#pragma once
#include "utils.hpp"
#include <cstdint>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <variant>
#include <vector>

namespace duck {

namespace fs = std::filesystem;

using EntryPreview = std::variant<std::string, ftxui::Element, std::monostate>;
using MenuInfo = std::tuple<std::string, size_t, std::vector<ftxui::Element>>;

struct FmgrEvent {
  enum class Type : std::uint8_t {
    UpdateCurrentDirectory,
    OpenFile,
    ToggleMark,
    ToggleHidden,
    Deletion,
    Creation,
    Rename,
    Paste,
    LoadDirectory,
    Refresh,
    Reload,
  } type_;
  fs::path path;
  fs::path path_to;
  std::vector<fs::path> paths;
  bool is_directory = false;
  bool is_cutting = false;
  std::string new_name;
};

struct RenderEvent {
  enum class Type : std::uint8_t {
    MoveSelectionDown,
    MoveSelectionUp,
    EnterDirectory,
    LeaveDirectory,
    ToggleNotification,
    ToggleDeletionDialog,
    ToggleRenameDialog,
    ToggleCreationDialog,
    ClearMarks,
    RefreshMenu,
    ReloadMenu,
    Quit,
  } type_;
};

struct PreviewUpdated {
  EntryPreview preview_;
};

struct DirecotryLoaded {
  Directory directory_;
};

using AppEvent =
    std::variant<FmgrEvent, RenderEvent, DirecotryLoaded, PreviewUpdated>;

template <typename... Ts> struct Visitor : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> Visitor(Ts...) -> Visitor<Ts...>;

} // namespace duck
