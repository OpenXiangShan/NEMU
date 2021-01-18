#include <common.h>

void ui_mainloop();
void init_device();

void engine_start() {
#ifndef USER_MODE
  /* Initialize devices. */
  init_device();
#endif

  /* Receive commands from user. */
  ui_mainloop();
}
