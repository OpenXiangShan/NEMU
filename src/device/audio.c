#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;
#ifndef __ICS_EXPORT
static int tail = 0;
#endif

static inline void audio_play(void *userdata, uint8_t *stream, int len) {
#ifndef __ICS_EXPORT
  int nread = len;
  int count = audio_base[reg_count];
  if (count < len) nread = count;

  if (nread + tail < CONFIG_SB_SIZE) {
    memcpy(stream, sbuf + tail, nread);
    tail += nread;
  } else {
    int first_cpy_len = CONFIG_SB_SIZE - tail;
    memcpy(stream, sbuf + tail, first_cpy_len);
    memcpy(stream + first_cpy_len, sbuf, nread - first_cpy_len);
    tail = nread - first_cpy_len;
  }
  audio_base[reg_count] -= nread;
  if (len > nread) memset(stream + nread, 0, len - nread);
#endif
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
#ifndef __ICS_EXPORT
  if (offset == reg_init * sizeof(uint32_t) && len == 4 && is_write) {
    SDL_AudioSpec s = {};
    s.freq = audio_base[reg_freq];
    s.format = AUDIO_S16SYS;
    s.channels = audio_base[reg_channels];
    s.samples = audio_base[reg_samples];
    s.callback = audio_play;
    s.userdata = NULL;

    tail = 0;
    audio_base[reg_count] = 0;
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_OpenAudio(&s, NULL);
    SDL_PauseAudio(0);
  }
#endif
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#ifndef __ICS_EXPORT
  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
