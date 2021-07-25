#include <cpu/cpu.h>

void ui_mainloop();

void engine_start() {
#ifdef CONFIG_AM
  cpu_exec(-1);
#else
  /* Receive commands from user. */
  ui_mainloop();
#endif
}
