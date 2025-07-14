#include "input_handler.h"
#include <notcurses/notcurses.h>

namespace duck {
input_handler::input_handler(notcurses *nc) : nc_{nc} {}

key_command input_handler::handle_input() {
  int id = notcurses_get_blocking(nc_, &input_);
  if (id == 'q' || id == NCKEY_ESC) {
    return key_command::QUIT;
  } else if (id == NCKEY_DOWN || id == 'j') {
    return key_command::MOVEDOWN;
  } else if (id == NCKEY_UP || id == 'k') {
    return key_command::MOVEUP;
  } else if (id == NCKEY_ENTER || id == 'l') {
    return key_command::DIRECTORDOWN;
  } else if (id == 'h') {
    return key_command::DIRECTORYUP;
  } else if (id == NCKEY_RESIZE) {
    return key_command::RESIZE;
  }
  return key_command::ERROR;
}
} // namespace duck
