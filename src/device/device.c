#include <common.h>
#include <utils.h>
#include <device/alarm.h>
#include <SDL2/SDL.h>

void init_serial();
void init_timer();
void init_alarm();
void init_vga();
void init_i8042();
void init_audio();
void init_disk();

void send_key(uint8_t, bool);
void vga_update_screen();

static int device_update_flag = false;

static void set_device_update_flag() {
  device_update_flag = true;
}

void device_update() {
  if (!device_update_flag) {
    return;
  }
  device_update_flag = false;
  ONDEF(CONFIG_HAS_VGA, vga_update_screen());

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        nemu_state.state = NEMU_QUIT;
        break;
#ifdef CONFIG_HAS_KEYBOARD
      // If a key was pressed
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        uint8_t k = event.key.keysym.scancode;
        bool is_keydown = (event.key.type == SDL_KEYDOWN);
        send_key(k, is_keydown);
        break;
      }
#endif
      default: break;
    }
  }
}

void sdl_clear_event_queue() {
  SDL_Event event;
  while (SDL_PollEvent(&event));
}

void init_device() {
  ONDEF(CONFIG_HAS_SERIAL, init_serial());
  ONDEF(CONFIG_HAS_TIMER, init_timer());
  ONDEF(CONFIG_HAS_VGA, init_vga());
  ONDEF(CONFIG_HAS_KEYBOARD, init_i8042());
  ONDEF(CONFIG_HAS_AUDIO, init_audio());
  ONDEF(CONFIG_HAS_DISK, init_disk());

  add_alarm_handle(set_device_update_flag);
  init_alarm();
}
