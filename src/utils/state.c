#include <utils.h>

NEMUState nemu_state = { .state = NEMU_STOP };

int is_exit_status_bad() {
  int good = (nemu_state.state == NEMU_END && nemu_state.halt_ret == 0) ||
    (nemu_state.state == NEMU_QUIT);
  if (!good) {
    Log("NEMU exit with bad state: %i, halt ret: %i", nemu_state.state, nemu_state.halt_ret);
  } else {
    Log("NEMU exit with good state: %i, halt ret: %i", nemu_state.state, nemu_state.halt_ret);
  }
  return !good;
}
