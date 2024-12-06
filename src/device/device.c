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
#include <device/alarm.h>
#ifndef CONFIG_SHARE
#if defined(CONFIG_HAS_AUDIO) || defined(CONFIG_HAS_VGA) || defined(CONFIG_HAS_KEYBOARD)
#include <SDL2/SDL.h>
#endif // defined(CONFIG_HAS_AUDIO) || defined(CONFIG_HAS_VGA) || defined(CONFIG_HAS_KEYBOARD)
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

static void set_device_update_flag() {
  device_update_flag = true;
}

void device_update() {
#ifndef CONFIG_SHARE
  if (!device_update_flag) {
    return;
  }
  device_update_flag = false;
  IFDEF(CONFIG_HAS_VGA, vga_update_screen());

#if defined(CONFIG_HAS_AUDIO) || defined(CONFIG_HAS_VGA) || defined(CONFIG_HAS_KEYBOARD)
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
#endif // CONFIG_HAS_KEYBOARD
      default: break;
    }
  }
#endif // defined(CONFIG_HAS_AUDIO) || defined(CONFIG_HAS_VGA) || defined(CONFIG_HAS_KEYBOARD)
#endif // CONFIG_SHARE
}

void sdl_clear_event_queue() {
#ifndef CONFIG_SHARE
#if defined(CONFIG_HAS_AUDIO) || defined(CONFIG_HAS_VGA) || defined(CONFIG_HAS_KEYBOARD)
  SDL_Event event;
  while (SDL_PollEvent(&event));
#endif // defined(CONFIG_HAS_AUDIO) || defined(CONFIG_HAS_VGA) || defined(CONFIG_HAS_KEYBOARD)
#endif // CONFIG_SHARE
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
  IFDEF(CONFIG_HAS_FLASH, init_flash());

  // host alarm for device and timer update.
  add_alarm_handle(set_device_update_flag);
  IFNDEF(CONFIG_DISABLE_HOST_ALARM, init_alarm());
}
