#include "file_manager.hpp"
#include "app_event.hpp"
#include "scheduler.hpp"
#include "stdexec/__detail/__execution_fwd.hpp"
#include "utils.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <string>

namespace duck {

namespace fs = std::filesystem;

constexpr size_t dirs_reserve = 256;

FileManager::FileManager(EventBus &event_bus) : event_bus_(event_bus) {}

Directory FileManager::load_directory(const fs::path &path) {
  Directory directory{.path_ = path};
  directory.entries_.reserve(dirs_reserve);
  directory.hidden_entries_.reserve(dirs_reserve);

  for (auto entry : fs::directory_iterator(
           path, fs::directory_options::skip_permission_denied)) {
    if (entry.path().empty()) {
      continue;
    }
    if (entry.path().filename().native()[0] == '.') {
      directory.hidden_entries_.push_back(std::move(entry));
    } else {
      directory.entries_.push_back(std::move(entry));
    }
  }

  std::ranges::sort(directory.entries_, entries_sorter);
  std::ranges::sort(directory.hidden_entries_, entries_sorter);
  return directory;
}

void FileManager::async_load_directory(const fs::path &path) {
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([path]() { return load_directory(path); }) |
              stdexec::then([this](const Directory &directory) {
                event_bus_.push_event(DirecotryLoaded{directory});
              });
  scope_.spawn(std::move(task));
}

void FileManager::async_update_preview(const fs::directory_entry &entry,
                                       const std::pair<int, int> &size) {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([this, entry, size]() -> std::optional<EntryPreview> {
        if (entry.is_directory()) {
          event_bus_.push_event(DirecotryLoaded{load_directory(entry)});
          event_bus_.push_event(DirectoryPreviewRequested{entry});
          return std::nullopt;
        }

        std::ifstream file(entry.path());
        if (!file.is_open()) {
          return "[Can't open file]";
        }

        auto [width, height] = size;
        std::string content;
        std::string line;
        for (int i = 0; i < height && std::getline(file, line); ++i) {
          if (line.size() > width) {
            line = line.substr(0, width) + "...";
          }
          content += line + '\n';
        }

        if (file.eof() && content.empty() && line.empty()) {
          return "[Empty file]";
        }

        return content;
      }) |
      stdexec::then([this](const std::optional<EntryPreview> &preview) {
        if (preview) {
          event_bus_.push_event(PreviewUpdated{preview.value()});
        }
      });
  scope_.spawn(task);
}

void FileManager::async_update_current_directory(const fs::path &path) {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([this, path]() {
        event_bus_.push_event(DirecotryLoaded{load_directory(path)});
      }) |
      stdexec::then([this, path]() {
        event_bus_.push_event(FmgrEvent{
            .type_ = FmgrEvent::Type::UpdateCurrentDirectory, .path = path});
      });
  scope_.spawn(task);
}

void FileManager::async_delete_entries(const std::vector<fs::path> &paths) {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) | stdexec::then([paths]() {
        for (const auto &path : paths) {
          if (fs::is_directory(path)) {
            fs::remove_all(path);
          } else {
            fs::remove(path);
          }
        }
      }) |
      stdexec::then([this]() {
        event_bus_.push_event(FmgrEvent{.type_ = FmgrEvent::Type::Reload});
      });
  scope_.spawn(std::move(task));
}

void FileManager::async_create_entry(const fs::path &path, bool is_directory) {
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then([path, is_directory]() {
                if (is_directory) {
                  fs::create_directories(path);
                } else {
                  std::ofstream ofs(path);
                  ofs.close();
                }
              }) |
              stdexec::then([this, path]() {
                event_bus_.push_event(FmgrEvent{
                    .type_ = FmgrEvent::Type::CreationSuccess, .path = path});
              });
  scope_.spawn(std::move(task));
}

void FileManager::async_rename_entry(const fs::path &old_path,
                                     const fs::path &new_path) {
  auto task = stdexec::schedule(Scheduler::io_scheduler()) |
              stdexec::then(
                  [old_path, new_path]() { fs::rename(old_path, new_path); }) |
              stdexec::then([this, old_path, new_path]() {
                event_bus_.push_event(FmgrEvent{
                    .type_ = FmgrEvent::Type::RenameSuccess,
                    .path = old_path,
                    .path_to = new_path,
                });
              });
  scope_.spawn(std::move(task));
}

void FileManager::async_paste_entries(const fs::path &dest,
                                      const std::vector<fs::path> &sources,
                                      bool is_cut) {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([dest, sources, is_cut]() {
        for (const auto &src : sources) {
          fs::path dest_path = dest / src.filename();
          if (is_cut) {
            fs::rename(src, dest_path);
          } else {
            fs::copy(src, dest_path, fs::copy_options::recursive);
          }
        }
      }) |
      stdexec::then([this]() {
        event_bus_.push_event(FmgrEvent{.type_ = FmgrEvent::Type::Reload});
      });
  scope_.spawn(std::move(task));
}

} // namespace duck
