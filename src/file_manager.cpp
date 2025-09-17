#include "file_manager.hpp"
#include "app_event.hpp"
#include "scheduler.hpp"
#include "utils.hpp"
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace duck {
using std::string;

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
                event_bus_.push_event(DirecotryLoaded{.update_preview_ = false,
                                                      .directory_ = directory});
              });
  scope_.spawn(std::move(task));
}

void FileManager::async_update_preview(const fs::directory_entry &entry,
                                       const std::pair<int, int> &size) {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([this, entry, size]() -> std::optional<std::string> {
        if (entry.is_directory()) {
          event_bus_.push_event(DirecotryLoaded{
              .update_preview_ = true, .directory_ = load_directory(entry)});
          return std::nullopt;
        }

        std::ifstream file(entry.path(), std::ios::binary);
        if (!file.is_open()) {
          return "[Can't open file]";
        }

        auto mime = get_mime(entry.path());
        if (!mime.starts_with("text/") &&
            !mime.starts_with("application/json")) {
          return "[Binary file]";
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
      stdexec::then([this](const std::optional<std::string> &preview) {
        if (preview) {
          event_bus_.push_event(TextPreview{preview.value()});
        }
      });
  scope_.spawn(task);
}

void FileManager::async_enter_directory(const fs::path &path) {
  auto task =
      stdexec::schedule(Scheduler::io_scheduler()) |
      stdexec::then([this, path]() {
        event_bus_.push_event(DirecotryLoaded{
            .update_preview_ = false, .directory_ = load_directory(path)});
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
          if (fs::exists(dest_path)) {
            const auto parent_path = dest_path.parent_path();
            const auto stem = dest_path.stem();
            const auto extension = dest_path.extension();
            int i = 1;
            while (fs::exists(dest_path)) {
              dest_path =
                  parent_path / (stem.string() + "_(" + std::to_string(i++) +
                                 ")" + extension.string());
            }
          }

          if (is_cut) {
            fs::rename(src, dest_path);
          } else {
            fs::copy(src, dest_path, fs::copy_options::recursive);
          }
        }
      }) |
      stdexec::then([this, dest]() {
        event_bus_.push_event(DirecotryLoaded{
            .update_preview_ = false, .directory_ = load_directory(dest)});
        event_bus_.push_event(FmgrEvent{
            .type_ = FmgrEvent::Type::UpdateCurrentDirectory, .path = dest});
      });
  scope_.spawn(std::move(task));
}

std::string FileManager::get_mime(const std::filesystem::path &path) {
  constexpr int buffer_size = 256;
  std::array<char, buffer_size> buffer{};
  std::string cmd = "file -b --mime-type " + path.string();

  using file_closer_t = int (*)(FILE *);
  std::unique_ptr<FILE, file_closer_t> pipe(popen(cmd.c_str(), "r"), pclose);

  if (!pipe) {
    return {"Unknow"};
  }

  if (fgets(buffer.data(), buffer.size(), pipe.get()) == nullptr) {
    throw std::runtime_error("fgets() failed");
  }

  return {buffer.data()};
}

} // namespace duck
