#include <dlfcn.h>
#include <isa.h>
#include <memory/paddr.h>
#include <stdlib.h>
#include "tran.h"

void (*rv64_memcpy_from_frontend)(paddr_t dest, void *src, size_t n) = NULL;
void (*rv64_getregs)(void *c) = NULL;
void (*rv64_setregs)(const void *c) = NULL;
void (*rv64_exec)(uint64_t n) = NULL;
void rv64_exec_code(uint64_t pc, int nr_instr);
void rv64_guest_setregs(void *cpu);
void init_rv64_reg();

static void init_rv64_interpreter() {
  char so_file[256];
  sprintf(so_file, "%s/build/riscv64-nemu-interpreter-so", getenv("NEMU_HOME"));

  void *handle;
  handle = dlopen(so_file, RTLD_LAZY | RTLD_DEEPBIND);
  assert(handle);

  rv64_memcpy_from_frontend = dlsym(handle, "difftest_memcpy_from_dut");
  assert(rv64_memcpy_from_frontend);

  rv64_getregs = dlsym(handle, "difftest_getregs");
  assert(rv64_getregs);

  rv64_setregs = dlsym(handle, "difftest_setregs");
  assert(rv64_setregs);

  rv64_exec = dlsym(handle, "difftest_exec");
  assert(rv64_exec);

  void (*rv64_init)(int port) = dlsym(handle, "difftest_init");
  assert(rv64_init);

  void (*rv64_init_device)() = dlsym(handle, "init_device");
  assert(rv64_init_device);

  rv64_init(0);
  rv64_init_device();
  rv64_memcpy_from_frontend(0, guest_to_host(0), PMEM_SIZE);
}

// this is to handle exceptions such as misaligned memory accessing
void load_bbl() {
  char bbl_file[256];
  sprintf(bbl_file, "%s/resource/bbl/build/bbl.bin", getenv("NEMU_HOME"));

  FILE *fp = fopen(bbl_file, "rb");
  if (fp == NULL) Log("If bbl is not built, run `make` under $(NEMU_HOME)/resource/bbl");
  Assert(fp, "Can not open '%s'", bbl_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  Assert(size < BBL_MAX_SIZE, "bbl size = %ld is too large", size);

  void *buf = malloc(size);
  assert(buf);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(buf, size, 1, fp);
  assert(ret == 1);

  fclose(fp);

  rv64_memcpy_from_frontend(0, buf, size);
  free(buf);
}

void init_engine() {
  init_rv64_interpreter();
  load_bbl();
  // execute enough instructions to set mtvec in bbl
  rv64_exec_code(0, 100);
  rv64_guest_setregs(&cpu);
  init_rv64_reg();
}
