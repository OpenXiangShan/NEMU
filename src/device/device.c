/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <utils.h>
#ifndef CONFIG_SHARE
#include <device/alarm.h>
#include <SDL2/SDL.h>
#endif // CONFIG_SHARE

void init_serial();
void init_uartlite();
void init_uart_snps();
void init_plic();
void init_timer();
void init_alarm();
void init_vga();
void init_i8042();
void init_audio();
void init_disk();
void init_sdcard();
void init_flash();
void load_flash_contents(const char *);

void send_key(uint8_t, bool);
void vga_update_screen();

static int device_update_flag = false;

#ifndef CONFIG_SHARE
static void set_device_update_flag() {
  device_update_flag = true;
}
#endif // CONFIG_SHARE

void device_update() {
  if (!device_update_flag) {
    return;
  }
  device_update_flag = false;
  IFDEF(CONFIG_HAS_VGA, vga_update_screen());

#ifndef CONFIG_SHARE
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
#endif
}

void sdl_clear_event_queue() {
#ifndef CONFIG_SHARE
  SDL_Event event;
  while (SDL_PollEvent(&event));
#endif
}

void init_device() {
  IFDEF(CONFIG_HAS_SERIAL, init_serial());
  IFDEF(CONFIG_HAS_UARTLITE, init_uartlite());
  IFDEF(CONFIG_HAS_UART_SNPS, init_uart_snps());
  IFDEF(CONFIG_HAS_PLIC, init_plic());
  IFDEF(CONFIG_HAS_TIMER, init_timer());
  IFDEF(CONFIG_HAS_VGA, init_vga());
  IFDEF(CONFIG_HAS_KEYBOARD, init_i8042());
  IFDEF(CONFIG_HAS_AUDIO, init_audio());
  IFDEF(CONFIG_HAS_DISK, init_disk());
  IFDEF(CONFIG_HAS_SDCARD, init_sdcard());
#ifndef CONFIG_SHARE
  IFDEF(CONFIG_HAS_FLASH, load_flash_contents(CONFIG_FLASH_IMG_PATH));
  IFDEF(CONFIG_HAS_FLASH, init_flash());
#endif

#ifndef CONFIG_SHARE
  add_alarm_handle(set_device_update_flag);
  init_alarm();
#endif
}
