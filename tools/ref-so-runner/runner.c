/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
  DIFFTEST_TO_DUT = 0,
  DIFFTEST_TO_REF = 1,
};

enum {
  NEMU_RUNNING = 0,
  NEMU_STOP = 1,
  NEMU_END = 2,
  NEMU_ABORT = 3,
  NEMU_QUIT = 4,
};

typedef struct {
  int state;
  uint64_t halt_pc;
  uint32_t halt_ret;
} NEMUState;

typedef void (*difftest_init_fn)(void);
typedef void (*difftest_exec_fn)(uint64_t n);
typedef void (*difftest_memcpy_fn)(uint64_t nemu_addr, void *dut_buf, size_t n, bool direction);
typedef void (*difftest_load_flash_fn)(void *flash_bin, size_t size);
typedef void (*difftest_set_ramsize_fn)(size_t ram_size);
typedef void (*difftest_close_fn)(void);
typedef uint64_t (*get_abs_instr_count_fn)(void);

typedef struct {
  const char *so_path;
  const char *image_path;
  uint64_t load_addr;
  uint64_t max_instructions;
  size_t ram_size;
} RunnerConfig;

typedef struct {
  void *handle;
  difftest_init_fn init;
  difftest_exec_fn exec;
  difftest_memcpy_fn memcpy;
  difftest_load_flash_fn load_flash;
  difftest_close_fn close;
  get_abs_instr_count_fn get_abs_instr_count;
  NEMUState *state;
} RefApi;

static void usage(const char *prog) {
  fprintf(stderr,
      "Usage: %s -b IMAGE [options]\n"
      "\n"
      "Options:\n"
      "  -b                      Accepted for NEMU CLI compatibility.\n"
      "  --so PATH               NEMU shared object, default $NEMU_HOME/build/riscv64-nemu-interpreter-so.\n"
      "  --load-addr ADDR          Guest physical load address, default 0x80000000.\n"
      "  --max-instructions NUM    Stop with an error after NUM instructions, default 1000000000.\n"
      "  -I NUM                  Alias of --max-instructions.\n"
      "  --ram-size BYTES          Call difftest_set_ramsize before init.\n"
      "  -h, --help                Show this help.\n",
      prog);
}

static uint64_t parse_u64(const char *text, const char *name) {
  char *end = NULL;
  errno = 0;
  uint64_t value = strtoull(text, &end, 0);
  if (errno != 0 || end == text || *end != '\0') {
    fprintf(stderr, "Invalid %s: %s\n", name, text);
    exit(2);
  }
  return value;
}

static char *make_default_so_path(void) {
  const char *nemu_home = getenv("NEMU_HOME");
  if (nemu_home == NULL || nemu_home[0] == '\0') {
    nemu_home = ".";
  }
  const char *suffix = "/build/riscv64-nemu-interpreter-so";
  size_t size = strlen(nemu_home) + strlen(suffix) + 1;
  char *path = malloc(size);
  if (path == NULL) {
    fprintf(stderr, "Failed to allocate shared object path\n");
    exit(1);
  }
  snprintf(path, size, "%s%s", nemu_home, suffix);
  return path;
}

static RunnerConfig parse_args(int argc, char **argv) {
  RunnerConfig cfg = {
    .so_path = NULL,
    .image_path = NULL,
    .load_addr = 0x80000000ull,
    .max_instructions = 1000000000ull,
    .ram_size = 0,
  };

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];
    if (strcmp(arg, "-b") == 0) {
      continue;
    } else if (strcmp(arg, "--so") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing value for --so\n");
        exit(2);
      }
      cfg.so_path = argv[i];
    } else if (strcmp(arg, "--image") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing value for --image\n");
        exit(2);
      }
      cfg.image_path = argv[i];
    } else if (strcmp(arg, "--load-addr") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing value for --load-addr\n");
        exit(2);
      }
      cfg.load_addr = parse_u64(argv[i], "--load-addr");
    } else if (strcmp(arg, "--max-instructions") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing value for --max-instructions\n");
        exit(2);
      }
      cfg.max_instructions = parse_u64(argv[i], "--max-instructions");
    } else if (strcmp(arg, "-I") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing value for -I\n");
        exit(2);
      }
      cfg.max_instructions = parse_u64(argv[i], "-I");
    } else if (strcmp(arg, "--ram-size") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Missing value for --ram-size\n");
        exit(2);
      }
      cfg.ram_size = (size_t)parse_u64(argv[i], "--ram-size");
    } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      usage(argv[0]);
      exit(0);
    } else if (arg[0] != '-' && cfg.image_path == NULL) {
      cfg.image_path = arg;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", arg);
      usage(argv[0]);
      exit(2);
    }
  }

  if (cfg.so_path == NULL) {
    cfg.so_path = make_default_so_path();
  }
  if (cfg.image_path == NULL) {
    usage(argv[0]);
    exit(2);
  }
  return cfg;
}

static void *load_required_symbol(void *handle, const char *name) {
  dlerror();
  void *symbol = dlsym(handle, name);
  const char *error = dlerror();
  if (error != NULL || symbol == NULL) {
    fprintf(stderr, "Failed to load %s: %s\n", name, error ? error : "symbol is NULL");
    exit(1);
  }
  return symbol;
}

static void *load_optional_symbol(void *handle, const char *name) {
  dlerror();
  void *symbol = dlsym(handle, name);
  const char *error = dlerror();
  return error == NULL ? symbol : NULL;
}

static RefApi load_ref_api(const RunnerConfig *cfg) {
  RefApi api = {0};
  api.handle = dlopen(cfg->so_path, RTLD_LAZY | RTLD_DEEPBIND);
  if (api.handle == NULL) {
    fprintf(stderr, "Failed to load %s: %s\n", cfg->so_path, dlerror());
    exit(1);
  }

  api.init = (difftest_init_fn)load_required_symbol(api.handle, "difftest_init");
  api.exec = (difftest_exec_fn)load_required_symbol(api.handle, "difftest_exec");
  api.memcpy = (difftest_memcpy_fn)load_optional_symbol(api.handle, "difftest_memcpy_init");
  if (api.memcpy == NULL) {
    api.memcpy = (difftest_memcpy_fn)load_required_symbol(api.handle, "difftest_memcpy");
  }
  api.load_flash = (difftest_load_flash_fn)load_optional_symbol(api.handle, "difftest_load_flash");
  api.close = (difftest_close_fn)load_optional_symbol(api.handle, "difftest_close");
  api.get_abs_instr_count = (get_abs_instr_count_fn)load_optional_symbol(api.handle, "get_abs_instr_count");
  api.state = (NEMUState *)load_required_symbol(api.handle, "nemu_state");

  difftest_set_ramsize_fn set_ramsize =
      (difftest_set_ramsize_fn)load_optional_symbol(api.handle, "difftest_set_ramsize");
  if (cfg->ram_size != 0) {
    if (set_ramsize == NULL) {
      fprintf(stderr, "The loaded NEMU shared object does not export difftest_set_ramsize\n");
      exit(1);
    }
    set_ramsize(cfg->ram_size);
  }

  return api;
}

static uint8_t *read_file(const char *path, size_t *size) {
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
    exit(1);
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fprintf(stderr, "Failed to seek %s\n", path);
    exit(1);
  }
  long file_size = ftell(fp);
  if (file_size < 0) {
    fprintf(stderr, "Failed to get size of %s\n", path);
    exit(1);
  }
  rewind(fp);

  uint8_t *buf = malloc((size_t)file_size);
  if (buf == NULL && file_size > 0) {
    fprintf(stderr, "Failed to allocate %ld bytes\n", file_size);
    exit(1);
  }

  size_t read_size = fread(buf, 1, (size_t)file_size, fp);
  if (read_size != (size_t)file_size) {
    fprintf(stderr, "Failed to read %s\n", path);
    exit(1);
  }
  fclose(fp);
  *size = read_size;
  return buf;
}

static uint64_t monotonic_ns(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
    exit(1);
  }
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint64_t current_guest_instructions(const RefApi *api, uint64_t fallback) {
  if (api->get_abs_instr_count != NULL) {
    return api->get_abs_instr_count();
  }
  return fallback;
}

static void initialize_flash(const RefApi *api) {
  if (api->load_flash != NULL) {
    api->load_flash(NULL, 0);
  }
}

static void print_statistics(uint64_t guest_instructions, uint64_t elapsed_ns) {
  uint64_t frequency = elapsed_ns == 0 ? 0 : (guest_instructions * 1000000000ull) / elapsed_ns;
  printf("total guest instructions = %" PRIu64 "\n", guest_instructions);
  printf("simulation frequency = %" PRIu64 "\n", frequency);
}

static void cleanup_ref_api(const RefApi *api, uint8_t *image) {
  free(image);
  if (api->close != NULL) {
    api->close();
  }
  dlclose(api->handle);
}

int main(int argc, char **argv) {
  RunnerConfig cfg = parse_args(argc, argv);
  RefApi api = load_ref_api(&cfg);

  size_t image_size = 0;
  uint8_t *image = read_file(cfg.image_path, &image_size);

  api.init();
  initialize_flash(&api);
  api.memcpy(cfg.load_addr, image, image_size, DIFFTEST_TO_REF);

  uint64_t guest_instructions = 0;
  uint64_t start_ns = monotonic_ns();
  int exit_code = 0;
  while (api.state->state != NEMU_END && api.state->state != NEMU_ABORT) {
    guest_instructions = current_guest_instructions(&api, guest_instructions);
    if (guest_instructions >= cfg.max_instructions) {
      fprintf(stderr, "Reached max instruction limit: %" PRIu64 "\n", cfg.max_instructions);
      exit_code = 1;
      break;
    }
    api.exec(1);
    if (api.get_abs_instr_count == NULL) {
      guest_instructions += 1;
    }
  }
  uint64_t elapsed_ns = monotonic_ns() - start_ns;

  guest_instructions = current_guest_instructions(&api, guest_instructions);
  print_statistics(guest_instructions, elapsed_ns);

  if (api.state->state == NEMU_ABORT || api.state->halt_ret != 0) {
    fprintf(stderr, "NEMU stopped with bad trap at pc = 0x%" PRIx64 ", code = %" PRIu32 "\n",
        api.state->halt_pc, api.state->halt_ret);
    exit_code = 1;
  }

  cleanup_ref_api(&api, image);
  return exit_code;
}
