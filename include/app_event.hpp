#pragma once
#include <cstdint>
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

template <typename... Ts> struct EventVisitor : Ts... {
  using Ts::operator()...;
};
template <typename... Ts> EventVisitor(Ts...) -> EventVisitor<Ts...>;

// 事件数据结构体
} // namespace duck
