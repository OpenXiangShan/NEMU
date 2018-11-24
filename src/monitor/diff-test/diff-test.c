#include <dlfcn.h>

#include "nemu.h"
#include "monitor/monitor.h"
#include "monitor/diff-test.h"

static void (*ref_difftest_memcpy_from_dut)(paddr_t dest, void *src, size_t n);
static void (*ref_difftest_getregs)(void *c);
static void (*ref_difftest_setregs)(const void *c);
static void (*ref_difftest_exec)(uint64_t n);

static bool is_skip_ref;
static bool is_skip_dut;
static uint32_t eflags_skip_mask;

void difftest_skip_ref() { is_skip_ref = true; }
void difftest_skip_dut() { is_skip_dut = true; }
void difftest_skip_eflags(uint32_t mask) { eflags_skip_mask = mask; }

void init_difftest(char *ref_so_file, long img_size) {
#ifndef DIFF_TEST
  return;
#endif

  assert(ref_so_file != NULL);

  void *handle;
  handle = dlopen(ref_so_file, RTLD_LAZY | RTLD_DEEPBIND);
  assert(handle);

  ref_difftest_memcpy_from_dut = dlsym(handle, "difftest_memcpy_from_dut");
  assert(ref_difftest_memcpy_from_dut);

  ref_difftest_getregs = dlsym(handle, "difftest_getregs");
  assert(ref_difftest_getregs);

  ref_difftest_setregs = dlsym(handle, "difftest_setregs");
  assert(ref_difftest_setregs);

  ref_difftest_exec = dlsym(handle, "difftest_exec");
  assert(ref_difftest_exec);

  void (*ref_difftest_init)(void) = dlsym(handle, "difftest_init");
  assert(ref_difftest_init);

  Log("Differential testing: \33[1;32m%s\33[0m", "ON");
  Log("The result of every instruction will be compared with %s. "
      "This will help you a lot for debugging, but also significantly reduce the performance. "
      "If it is not necessary, you can turn it off in include/common.h.", ref_so_file);

  ref_difftest_init();
  ref_difftest_memcpy_from_dut(ENTRY_START, guest_to_host(ENTRY_START), img_size);
  ref_difftest_setregs(&cpu);
}

#define check_reg(regs, r) \
  if (regs.r != cpu.r) { \
    Log("%s is different after executing instruction at eip = 0x%08x, right = 0x%08x, wrong = 0x%08x", \
        str(r), eip, regs.r, cpu.r); \
  }

#define check_flag(regs, f) \
  do { \
    uint32_t mask = concat(EFLAGS_MASK_, f); \
    uint32_t __flag = ((regs.eflags & mask) != 0); \
    if (mask & eflags_skip_mask) { \
      cpu.f = __flag; \
    } \
    else { \
      if (__flag != cpu.f) { \
        Log("%s is different after executing instruction at eip = 0x%08x, right = 0x%08x, wrong = 0x%08x", \
            "eflags." str(f), eip, __flag, cpu.f); \
        nemu_state = NEMU_ABORT; \
      } \
    } \
  } while(0)

void difftest_step(uint32_t eip) {
  CPU_state ref_r;

  if (is_skip_dut) {
    is_skip_dut = false;
    return;
  }

  if (is_skip_ref) {
    // to skip the checking of an instruction, just copy the reg state to reference design
    ref_difftest_getregs(&ref_r);
    cpu.eflags = ref_r.eflags;
    ref_difftest_setregs(&cpu);
    is_skip_ref = false;
    return;
  }

  ref_difftest_exec(1);
  ref_difftest_getregs(&ref_r);

  // TODO: Check the registers state with QEMU.
  // Set `diff` as `true` if they are not the same.
  // TODO();
  if (memcmp(&cpu, &ref_r, DIFFTEST_REG_SIZE)) {
    check_reg(ref_r, eax);
    check_reg(ref_r, ecx);
    check_reg(ref_r, edx);
    check_reg(ref_r, ebx);
    check_reg(ref_r, esp);
    check_reg(ref_r, ebp);
    check_reg(ref_r, esi);
    check_reg(ref_r, edi);
    check_reg(ref_r, eip);

    nemu_state = NEMU_ABORT;
  }

  check_flag(ref_r, CF);
  check_flag(ref_r, OF);
  check_flag(ref_r, SF);
  check_flag(ref_r, ZF);

  if (eflags_skip_mask) {
    eflags_skip_mask = 0;
  }
}
