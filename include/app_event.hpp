#pragma once
#include <cstdint>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <variant>

namespace duck {

struct FmgrEvent {
  enum class Type : std::uint8_t {
    OpenFile,
    ToggleMark,
    ToggleHidden,
    Deletion,
    Creation,
    Rename,
    StartYanking,
    StartCutting,
    Paste,
  } type_;
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

using AppEvent = std::variant<FmgrEvent, RenderEvent>;
using EntryPreview = std::variant<std::string, ftxui::Element, std::monostate>;
using MenuInfo = std::tuple<std::string, int, std::vector<ftxui::Element>>;

template <typename... Ts> struct Visitor : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> Visitor(Ts...) -> Visitor<Ts...>;

// 事件数据结构体
} // namespace duck
