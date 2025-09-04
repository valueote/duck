#pragma once
#include "event_bus.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace duck {

namespace fs = std::filesystem;

class FileManager {
private:
  EventBus &event_bus_;

  void async_load_directory(const fs::path &path, bool use_cache);
  void async_delete_entries(const std::vector<fs::path> &paths);
  void async_create_entry(const fs::path &path, bool is_directory);
  void async_rename_entry(const fs::path &old_path, const fs::path &new_path);
  void async_paste_entries(const fs::path &dest, const std::vector<fs::path> &sources, bool is_cut);

public:
  FileManager(EventBus &event_bus);

  void handle_event(const FmgrEvent &event);
};

} // namespace duck
