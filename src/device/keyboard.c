#include <device/map.h>
#include <monitor/monitor.h>
#include <SDL2/SDL.h>

#define I8042_DATA_PORT 0x60
#define I8042_DATA_MMIO 0xa1000060
#define KEYBOARD_IRQ 1

static uint32_t *i8042_data_port_base = (uint32_t *) nullptr;

// Note that this is not the standard
#define _KEYS(f) \
  f(ESCAPE) f(F1) f(F2) f(F3) f(F4) f(F5) f(F6) f(F7) f(F8) f(F9) f(F10) f(F11) f(F12) \
f(GRAVE) f(1) f(2) f(3) f(4) f(5) f(6) f(7) f(8) f(9) f(0) f(MINUS) f(EQUALS) f(BACKSPACE) \
f(TAB) f(Q) f(W) f(E) f(R) f(T) f(Y) f(U) f(I) f(O) f(P) f(LEFTBRACKET) f(RIGHTBRACKET) f(BACKSLASH) \
f(CAPSLOCK) f(A) f(S) f(D) f(F) f(G) f(H) f(J) f(K) f(L) f(SEMICOLON) f(APOSTROPHE) f(RETURN) \
f(LSHIFT) f(Z) f(X) f(C) f(V) f(B) f(N) f(M) f(COMMA) f(PERIOD) f(SLASH) f(RSHIFT) \
f(LCTRL) f(APPLICATION) f(LALT) f(SPACE) f(RALT) f(RCTRL) \
f(UP) f(DOWN) f(LEFT) f(RIGHT) f(INSERT) f(DELETE) f(HOME) f(END) f(PAGEUP) f(PAGEDOWN)

#define _KEY_NAME(k) _KEY_##k,

enum {
  _KEY_NONE = 0,
  MAP(_KEYS, _KEY_NAME)
};

#define SDL_KEYMAP(k) [concat(SDL_SCANCODE_, k)] = concat(_KEY_, k),
static uint32_t keymap[256];

void init_keymap() {

  keymap[SDL_SCANCODE_ESCAPE] = _KEY_ESCAPE;
  keymap[SDL_SCANCODE_F1] = _KEY_F1;
  keymap[SDL_SCANCODE_F2] = _KEY_F2;
  keymap[SDL_SCANCODE_F3] = _KEY_F3;
  keymap[SDL_SCANCODE_F4] = _KEY_F4;
  keymap[SDL_SCANCODE_F5] = _KEY_F5;
  keymap[SDL_SCANCODE_F6] = _KEY_F6;
  keymap[SDL_SCANCODE_F7] = _KEY_F7;
  keymap[SDL_SCANCODE_F8] = _KEY_F8;
  keymap[SDL_SCANCODE_F9] = _KEY_F9;
  keymap[SDL_SCANCODE_F10] = _KEY_F10;
  keymap[SDL_SCANCODE_F11] = _KEY_F11;
  keymap[SDL_SCANCODE_F12] = _KEY_F12;
  keymap[SDL_SCANCODE_GRAVE] = _KEY_GRAVE;
  keymap[SDL_SCANCODE_1] = _KEY_1;
  keymap[SDL_SCANCODE_2] = _KEY_2;
  keymap[SDL_SCANCODE_3] = _KEY_3;
  keymap[SDL_SCANCODE_4] = _KEY_4;
  keymap[SDL_SCANCODE_5] = _KEY_5;
  keymap[SDL_SCANCODE_6] = _KEY_6;
  keymap[SDL_SCANCODE_7] = _KEY_7;
  keymap[SDL_SCANCODE_8] = _KEY_8;
  keymap[SDL_SCANCODE_9] = _KEY_9;
  keymap[SDL_SCANCODE_0] = _KEY_0;
  keymap[SDL_SCANCODE_MINUS] = _KEY_MINUS;
  keymap[SDL_SCANCODE_EQUALS] = _KEY_EQUALS;
  keymap[SDL_SCANCODE_BACKSPACE] = _KEY_BACKSPACE;
  keymap[SDL_SCANCODE_TAB] = _KEY_TAB;
  keymap[SDL_SCANCODE_Q] = _KEY_Q;
  keymap[SDL_SCANCODE_W] = _KEY_W;
  keymap[SDL_SCANCODE_E] = _KEY_E;
  keymap[SDL_SCANCODE_R] = _KEY_R;
  keymap[SDL_SCANCODE_T] = _KEY_T;
  keymap[SDL_SCANCODE_Y] = _KEY_Y;
  keymap[SDL_SCANCODE_U] = _KEY_U;
  keymap[SDL_SCANCODE_I] = _KEY_I;
  keymap[SDL_SCANCODE_O] = _KEY_O;
  keymap[SDL_SCANCODE_P] = _KEY_P;
  keymap[SDL_SCANCODE_LEFTBRACKET] = _KEY_LEFTBRACKET;
  keymap[SDL_SCANCODE_RIGHTBRACKET] = _KEY_RIGHTBRACKET;
  keymap[SDL_SCANCODE_BACKSLASH] = _KEY_BACKSLASH;
  keymap[SDL_SCANCODE_CAPSLOCK] = _KEY_CAPSLOCK;
  keymap[SDL_SCANCODE_A] = _KEY_A;
  keymap[SDL_SCANCODE_S] = _KEY_S;
  keymap[SDL_SCANCODE_D] = _KEY_D;
  keymap[SDL_SCANCODE_F] = _KEY_F;
  keymap[SDL_SCANCODE_G] = _KEY_G;
  keymap[SDL_SCANCODE_H] = _KEY_H;
  keymap[SDL_SCANCODE_J] = _KEY_J;
  keymap[SDL_SCANCODE_K] = _KEY_K;
  keymap[SDL_SCANCODE_L] = _KEY_L;
  keymap[SDL_SCANCODE_SEMICOLON] = _KEY_SEMICOLON;
  keymap[SDL_SCANCODE_APOSTROPHE] = _KEY_APOSTROPHE;
  keymap[SDL_SCANCODE_RETURN] = _KEY_RETURN;
  keymap[SDL_SCANCODE_LSHIFT] = _KEY_LSHIFT;
  keymap[SDL_SCANCODE_Z] = _KEY_Z;
  keymap[SDL_SCANCODE_X] = _KEY_X;
  keymap[SDL_SCANCODE_C] = _KEY_C;
  keymap[SDL_SCANCODE_V] = _KEY_V;
  keymap[SDL_SCANCODE_B] = _KEY_B;
  keymap[SDL_SCANCODE_N] = _KEY_N;
  keymap[SDL_SCANCODE_M] = _KEY_M;
  keymap[SDL_SCANCODE_COMMA] = _KEY_COMMA;
  keymap[SDL_SCANCODE_PERIOD] = _KEY_PERIOD;
  keymap[SDL_SCANCODE_SLASH] = _KEY_SLASH;
  keymap[SDL_SCANCODE_RSHIFT] = _KEY_RSHIFT;
  keymap[SDL_SCANCODE_LCTRL] = _KEY_LCTRL;
  keymap[SDL_SCANCODE_APPLICATION] = _KEY_APPLICATION;
  keymap[SDL_SCANCODE_LALT] = _KEY_LALT;
  keymap[SDL_SCANCODE_SPACE] = _KEY_SPACE;
  keymap[SDL_SCANCODE_RALT] = _KEY_RALT;
  keymap[SDL_SCANCODE_RCTRL] = _KEY_RCTRL;
  keymap[SDL_SCANCODE_UP] = _KEY_UP;
  keymap[SDL_SCANCODE_DOWN] = _KEY_DOWN;
  keymap[SDL_SCANCODE_LEFT] = _KEY_LEFT;
  keymap[SDL_SCANCODE_RIGHT] = _KEY_RIGHT;
  keymap[SDL_SCANCODE_INSERT] = _KEY_INSERT;
  keymap[SDL_SCANCODE_DELETE] = _KEY_DELETE;
  keymap[SDL_SCANCODE_HOME] = _KEY_HOME;
  keymap[SDL_SCANCODE_END] = _KEY_END;
  keymap[SDL_SCANCODE_PAGEUP] = _KEY_PAGEUP;
  keymap[SDL_SCANCODE_PAGEDOWN] = _KEY_PAGEDOWN;

}

#define KEY_QUEUE_LEN 1024
static int key_queue[KEY_QUEUE_LEN] = {};
static int key_f = 0, key_r = 0;

#define KEYDOWN_MASK 0x8000

void send_key(uint8_t scancode, nemu_bool is_keydown) {
  if (nemu_state.state == NEMU_RUNNING &&
      keymap[scancode] != _KEY_NONE) {
    uint32_t am_scancode = keymap[scancode] | (is_keydown ? KEYDOWN_MASK : 0);
    key_queue[key_r] = am_scancode;
    key_r = (key_r + 1) % KEY_QUEUE_LEN;
    Assert(key_r != key_f, "key queue overflow!");
  }
}

static void i8042_data_io_handler(uint32_t offset, int len, nemu_bool is_write) {
  assert(!is_write);
  assert(offset == 0);
  if (key_f != key_r) {
    i8042_data_port_base[0] = key_queue[key_f];
    key_f = (key_f + 1) % KEY_QUEUE_LEN;
  }
  else {
    i8042_data_port_base[0] = _KEY_NONE;
  }
}

void init_i8042() {
  i8042_data_port_base = (uint32_t *)new_space(4);
  i8042_data_port_base[0] = _KEY_NONE;
  add_pio_map("keyboard", I8042_DATA_PORT, (uint8_t *)i8042_data_port_base, 4, i8042_data_io_handler);
  add_mmio_map("keyboard", I8042_DATA_MMIO, (uint8_t *)i8042_data_port_base, 4, i8042_data_io_handler);
}
