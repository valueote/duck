#include <notcurses/notcurses.h>
namespace duck {

enum class key_command {
  MOVEUP,
  MOVEDOWN,
  DIRECTORYUP,
  DIRECTORDOWN,
  QUIT,
  RESIZE,
  ERROR
};

class input_handler {
private:
  notcurses *nc_;
  ncinput input_;

public:
  input_handler(notcurses *nc);
  key_command handle_input();
};

} // namespace duck
