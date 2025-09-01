#pragma once
#include <cstdint>
#include <variant>

namespace duck {

// 定义应用事件枚举

struct Operation {
  enum class Type : std::uint8_t {
    OpenFile,
    ToggleMark,
    Deletion,
    Creation,
    Rename,
    StartYanking,
    StartCutting,
    Paste,
    ToggleHidden,
  } type_;
};

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
    ClearMarks,
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
    ShowDeletionDialog,
    HideDeletionDialog,
    ShowCreationDialog,
    HideCreationDialog,
    ShowRenameDialog,
    HideRenameDialog,
    ShowNotification,
    HideNotification,
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
