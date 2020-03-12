void init_Icache();
void init_jmp();
void init_tl();
void ui_mainloop();

void init_engine() {
  init_Icache();
  init_jmp();
  init_tl();
}

void mainloop() {
  ui_mainloop();
}
