#include <common.h>

#ifdef CONFIG_HAS_VGA

#define SCREEN_W (ISDEF(CONFIG_VGA_SIZE_800x600) ? 800 : 400)
#define SCREEN_H (ISDEF(CONFIG_VGA_SIZE_800x600) ? 600 : 300)
#define SCREEN_SIZE ((SCREEN_H * SCREEN_W) * sizeof(uint32_t))

#include <device/map.h>
#include <SDL2/SDL.h>

static uint32_t (*vmem) [SCREEN_W] = NULL;
static uint32_t *vgactl_port_base = NULL;

#ifdef CONFIG_VGA_SHOW_SCREEN
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

static inline void init_screen() {
  SDL_Window *window = NULL;
  char title[128];
  sprintf(title, "%s-NEMU", str(__ISA__));
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(
      SCREEN_W * (ISDEF(CONFIG_VGA_SIZE_400x300) ? 2 : 1),
      SCREEN_H * (ISDEF(CONFIG_VGA_SIZE_400x300) ? 2 : 1),
      0, &window, &renderer);
  SDL_SetWindowTitle(window, title);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STATIC, SCREEN_W, SCREEN_H);
}

static inline void update_screen() {
  SDL_UpdateTexture(texture, NULL, vmem, SCREEN_W * sizeof(vmem[0][0]));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}
#endif

void vga_update_screen() {
#ifdef __ICS_EXPORT
  // TODO: call `update_screen()` when the sync register is non-zero,
  // then zero out the sync register
#else
  if (vgactl_port_base[1]) {
    ONDEF(CONFIG_VGA_SHOW_SCREEN, update_screen());
    vgactl_port_base[1] = 0;
  }
#endif
}

void init_vga() {
  ONDEF(CONFIG_VGA_SHOW_SCREEN, init_screen());

  vgactl_port_base = (void *)new_space(8);
  vgactl_port_base[0] = ((SCREEN_W) << 16) | (SCREEN_H);
  add_pio_map("screen", CONFIG_VGA_CTL_PORT, (void *)vgactl_port_base, 8, NULL);
  add_mmio_map("screen", CONFIG_VGA_CTL_MMIO, (void *)vgactl_port_base, 8, NULL);

  vmem = (void *)new_space(SCREEN_SIZE);
  add_mmio_map("vmem", CONFIG_FB_ADDR, (void *)vmem, SCREEN_SIZE, NULL);
}
#endif	/* CONFIG_HAS_VGA */
