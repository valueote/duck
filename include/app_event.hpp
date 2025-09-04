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

struct FmgrEvent {
  enum class Type : std::uint8_t {
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
  bool use_cache = true;
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
    UpdatePreview,
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

struct DirectoryLoaded {
  Directory directory;
};

using AppEvent = std::variant<FmgrEvent, RenderEvent, DirectoryLoaded>;
using EntryPreview = std::variant<std::string, ftxui::Element, std::monostate>;
using MenuInfo = std::tuple<std::string, int, std::vector<ftxui::Element>>;

template <typename... Ts> struct Visitor : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> Visitor(Ts...) -> Visitor<Ts...>;

// 事件数据结构体
} // namespace duck
