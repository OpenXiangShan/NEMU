void ui_mainloop();

void engine_start() {
  /* Receive commands from user. */
  ui_mainloop();
}
