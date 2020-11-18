#include <common.h>

#ifdef HAS_IOE

//#define SHOW_SCREEN

#include <device/map.h>
#include <SDL2/SDL.h>

#define SCREEN_H 300
#define SCREEN_W 400
#define SCREEN_SIZE ((SCREEN_H * SCREEN_W) * sizeof(uint32_t))

static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

static uint32_t (*vmem) [SCREEN_W] = NULL;
static uint32_t *screensize_port_base = NULL;

static inline void update_screen() {
  SDL_UpdateTexture(texture, NULL, vmem, SCREEN_W * sizeof(vmem[0][0]));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

static void vga_io_handler(uint32_t offset, int len, bool is_write) {
  if (offset == 4 && len == 4 && is_write) {
#ifdef SHOW_SCREEN
    update_screen();
#endif
  }
}

void init_vga() {
#ifdef SHOW_SCREEN
  SDL_Window *window = NULL;
  char title[128];
  sprintf(title, "%s-NEMU", str(__ISA__));
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(SCREEN_W * 2, SCREEN_H * 2, 0, &window, &renderer);
  SDL_SetWindowTitle(window, title);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STATIC, SCREEN_W, SCREEN_H);
#endif

  screensize_port_base = (void *)new_space(8);
  screensize_port_base[0] = ((SCREEN_W) << 16) | (SCREEN_H);
  add_pio_map("screen", SCREEN_PORT, (void *)screensize_port_base, 8, vga_io_handler);
  add_mmio_map("screen", SCREEN_MMIO, (void *)screensize_port_base, 8, vga_io_handler);

  vmem = (void *)new_space(0x80000);
  add_mmio_map("vmem", VMEM, (void *)vmem, SCREEN_SIZE, NULL);
}
#endif	/* HAS_IOE */
