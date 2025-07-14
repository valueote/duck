#include "fs_utils.h"
#include "ui.h"
#include <filesystem>
#include <vector>
namespace duck {
class file_manager {
private:
  ui ui_;
  fs_utils fs_utils_;
  ncinput input_handler_;
  bool running_;
  size_t selected_;

  fs::path current_path_;
  std::vector<fs::directory_entry> entries_;
  std::vector<fs::directory_entry> selected_dir_entries_;
  void move_selected_down();
  void move_selected_up();
  int wait_input();

public:
  file_manager();
  void run();
  void stop();
};

} // namespace duck
