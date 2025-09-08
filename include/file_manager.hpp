#pragma once
#include "event_bus.hpp"
#include "exec/async_scope.hpp"
#include "utils.hpp"
#include <filesystem>
#include <vector>

namespace duck {

namespace fs = std::filesystem;

class FileManager {
private:
  EventBus &event_bus_;
  exec::async_scope scope_;

public:
  static Directory load_directory(const fs::path &path);
  void async_load_directory(const fs::path &path);
  void async_update_current_directory(const fs::path &path);
  void async_update_preview(const fs::directory_entry &entry,
                            const std::pair<int, int> &size);
  void async_delete_entries(const std::vector<fs::path> &paths);
  void async_create_entry(const fs::path &path, bool is_directory);
  void async_rename_entry(const fs::path &old_path, const fs::path &new_path);
  void async_paste_entries(const fs::path &dest,
                           const std::vector<fs::path> &sources, bool is_cut);
  FileManager(EventBus &event_bus);
};

} // namespace duck
