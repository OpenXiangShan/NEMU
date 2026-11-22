/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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
#include <memory/image_loader.h>

#ifndef CONFIG_MODE_USER
#ifdef CONFIG_ZSTD_COMPRESS

#include <limits.h>
#include <memory/elfloader.h>
#include <memory/paddr.h>
#include <stdio.h>
#include <string.h>
#include <zstd.h>

static void copy_zstd_raw_chunk(uint8_t *dest, const uint64_t *words, size_t size) {
  // Avoid allocating guest RAM pages just to write zeros to already zeroed memory.
  size_t word_count = size / sizeof(*words);
  for (size_t i = 0; i < word_count; i++) {
    uint64_t old_word;
    memcpy(&old_word, dest, sizeof(old_word));
    if (words[i] != 0 || old_word != 0) {
      memcpy(dest, &words[i], sizeof(*words));
    }
    dest += sizeof(*words);
  }
  memcpy(dest, words + word_count, size % sizeof(*words));
}

long load_zstd_img(const char *filename, uint8_t *load_start, size_t img_size) {
  Log("Loading Zstd image %s", filename);
  FILE *source = fopen(filename, "rb");
  if (!source) {
    perror(filename);
    return -1;
  }

  ZSTD_DStream *stream = ZSTD_createDStream();
  if (!stream) {
    fprintf(stderr, "Cannot create Zstd decoder for '%s'\n", filename);
    fclose(source);
    return -1;
  }

  uint8_t compressed[64 * 1024];
  uint64_t decoded[8192];
  ZSTD_inBuffer input = {compressed, 0, 0};
  ZSTD_outBuffer output = {decoded, sizeof(decoded), 0};
  size_t remaining = ZSTD_initDStream(stream);
  size_t load_size = img_size ? img_size : MEMORY_SIZE;
  size_t raw_size = 0;
  FILE *elf_file = NULL;
  long loaded_size = -1;

  for (;;) {
    if (input.pos == input.size) {
      input.size = fread(compressed, 1, sizeof(compressed), source);
      input.pos = 0;
      if (ferror(source)) {
        perror(filename);
        goto done;
      }
    }

    bool finished = input.size == 0 && remaining == 0;
    if (!finished) {
      size_t input_before = input.pos;
      size_t output_before = output.pos;
      remaining = ZSTD_decompressStream(stream, &output, &input);
      if (ZSTD_isError(remaining)) {
        fprintf(stderr, "Cannot decompress '%s': %s\n", filename, ZSTD_getErrorName(remaining));
        goto done;
      }
      if (remaining != 0 && input.pos == input_before && output.pos == output_before) {
        fprintf(stderr, "Incomplete Zstd stream in '%s'\n", filename);
        goto done;
      }
      if (output.pos < output.size) {
        continue;
      }
    }

    // Wait for a full chunk or EOF before checking the decoded ELF header.
    if (!elf_file && raw_size == 0 && is_elf_image((const uint8_t *)decoded, output.pos)) {
      elf_file = tmpfile();
      if (!elf_file) {
        perror("Cannot create temporary ELF backing file");
        goto done;
      }
    }

    if (elf_file) {
      if (fwrite(decoded, 1, output.pos, elf_file) != output.pos) {
        perror("Cannot write temporary ELF backing file");
        goto done;
      }
    } else {
      if (output.pos > load_size - raw_size) {
        fprintf(stderr, "Decompressed image '%s' exceeds memory capacity\n", filename);
        goto done;
      }
      if (output.pos != 0) {
        copy_zstd_raw_chunk(load_start + raw_size, decoded, output.pos);
        raw_size += output.pos;
      }
    }
    if (finished) {
      break;
    }
    output.pos = 0;
  }

  if (elf_file) {
    if (fflush(elf_file) != 0) {
      perror("Cannot flush temporary ELF backing file");
      goto done;
    }
    loaded_size = load_elf_image_fd(fileno(elf_file), filename, load_start, load_size, CONFIG_MBASE);
    if (loaded_size >= 0) {
      Log("Loaded Zstd-compressed ELF image %s", filename);
    }
  } else if (raw_size <= LONG_MAX) {
    loaded_size = raw_size;
  }

done:
  if (elf_file) {
    fclose(elf_file);
  }
  ZSTD_freeDStream(stream);
  fclose(source);
  return loaded_size;
}

#else

long load_zstd_img(const char *filename, uint8_t *load_start, size_t img_size) {
  panic("Enable CONFIG_ZSTD_COMPRESS and rebuild NEMU to load '%s'", filename);
  return -1;
}

#endif  // CONFIG_ZSTD_COMPRESS
#endif  // CONFIG_MODE_USER
