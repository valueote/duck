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
  struct input_state {
    uint32_t last_key = 0;
    ncintype_e last_type = NCTYPE_UNKNOWN;
    uint64_t last_time = 0;
  };

  input_state input_state_;

  fs::path current_path_;
  std::vector<fs::directory_entry> curdir_entries;
  std::vector<fs::directory_entry> selected_dir_entries_;
  void move_selected_down();
  void move_selected_up();
  int wait_input();
  int get_input_nblock();
  void handle_input(int id);

  void update_curdir_entries();
  void update_selected_entries();

public:
  file_manager();
  void run();
  void stop();
};

} // namespace duck
