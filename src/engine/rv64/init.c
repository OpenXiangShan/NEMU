#define _GNU_SOURCE
#include <dlfcn.h>
#include <isa.h>
#include <isa/riscv64.h>
#include <memory/paddr.h>
#include <stdlib.h>
#include "tran.h"
#include <monitor/difftest.h>

void (*backend_memcpy)(paddr_t dest, void *src, size_t n, bool to_backend) = NULL;
void (*backend_regcpy)(void *c, bool to_backend) = NULL;
void (*backend_exec)(uint64_t n) = NULL;

void backend_exec_code(uint64_t pc, int nr_instr);
void guest_getregs(CPU_state *cpu);
void spill_init();
void guest_init();
void tran_mainloop();

static void init_rv64_interpreter() {
  char so_file[256];
  sprintf(so_file, "%s/build/riscv64-nemu-interpreter-so", getenv("NEMU_HOME"));

  void *handle;
  handle = dlmopen(LM_ID_NEWLM, so_file, RTLD_LAZY | RTLD_DEEPBIND);
  assert(handle);

  backend_memcpy = dlsym(handle, "difftest_memcpy");
  assert(backend_memcpy);

  backend_regcpy = dlsym(handle, "difftest_regcpy");
  assert(backend_regcpy);

  backend_exec = dlsym(handle, "difftest_exec");
  assert(backend_exec);

  void (*backend_init)(int port) = dlsym(handle, "difftest_init");
  assert(backend_init);

  void (*backend_init_device)() = dlsym(handle, "init_device");
  assert(backend_init_device);

  // initialize serial before the dummy serial in difftest_init()
  backend_init_device();
  backend_init(0);
  backend_memcpy(PMEM_BASE, guest_to_host(0), PMEM_SIZE, true);
}

// this is to handle exceptions such as misaligned memory accessing
static void load_bbl() {
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

  backend_memcpy(PMEM_BASE, buf, size, true);
  free(buf);
}

static void init_rv64_reg() {
  riscv64_CPU_state r;
  backend_regcpy(&r, false);
  r.gpr[mask32]._64 = 0x00000000fffffffful;
  r.gpr[mask16]._64 = 0x000000000000fffful;
  if (spm_base != 0) r.gpr[spm_base]._64 = riscv64_PMEM_BASE;
  r.gpr[0]._64 = 0x0;
  backend_regcpy(&r, true);
}

void engine_start() {
  init_rv64_interpreter();
  load_bbl();
  // execute enough instructions to set mtvec in bbl
  backend_exec_code(riscv64_PMEM_BASE, 100);
  guest_init();
  init_rv64_reg();
  spill_init();
  guest_getregs(&cpu);
  ONDEF(CONFIG_DIFFTEST, ref_difftest_regcpy(&cpu, true));

  tran_mainloop();
}
