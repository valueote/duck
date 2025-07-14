#pragma once
#include <filesystem>
#include <vector>
namespace duck {
namespace fs = std::filesystem;
class fs_utils {
private:
  fs::path current_path_;
  std::vector<fs::directory_entry> entries_;

public:
  fs_utils();
  fs::path get_current_path();
  void update_current_path();
  void update_entries();
  std::vector<fs::directory_entry> get_direcotry_entries();
};

} // namespace duck
