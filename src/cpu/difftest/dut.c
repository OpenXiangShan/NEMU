/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include <dlfcn.h>

#include <isa.h>
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <utils.h>
#include <difftest.h>

void (*ref_difftest_memcpy)(paddr_t addr, void *buf, size_t n, bool direction) = NULL;
void (*ref_difftest_regcpy)(void *dut, bool direction) = NULL;
void (*ref_difftest_exec)(uint64_t n) = NULL;
void (*ref_difftest_raise_intr)(uint64_t NO) = NULL;
int  (*ref_difftest_store_commit)(uint64_t *addr, uint64_t *data, uint8_t *mask) = NULL;
#ifdef CONFIG_DIFFTEST

IFDEF(CONFIG_DIFFTEST_REF_QEMU_DL, __thread uint8_t resereve_for_qemu_tls[4096]);

static bool is_skip_ref = false;
static int skip_dut_nr_instr = 0;
void (*patch_fn)(void *arg) = NULL;
static void* patch_arg = NULL;
#ifndef __ICS_EXPORT
static bool is_detach = false;
#endif

// this is used to let ref skip instructions which
// can not produce consistent behavior with NEMU
void difftest_skip_ref() {
#ifndef __ICS_EXPORT
  if (is_detach) return;
#endif
  is_skip_ref = true;
  // If such an instruction is one of the instruction packing in QEMU
  // (see below), we end the process of catching up with QEMU's pc to
  // keep the consistent behavior in our best.
  // Note that this is still not perfect: if the packed instructions
  // already write some memory, and the incoming instruction in NEMU
  // will load that memory, we will encounter false negative. But such
  // situation is infrequent.
  skip_dut_nr_instr = 0;
}

// this is used to deal with instruction packing in QEMU.
// Sometimes letting QEMU step once will execute multiple instructions.
// We should skip checking until NEMU's pc catches up with QEMU's pc.
// The semantic is
//   Let REF run `nr_ref` instructions first.
//   We expect that DUT will catch up with REF within `nr_dut` instructions.
void difftest_skip_dut(int nr_ref, int nr_dut) {
#ifndef __ICS_EXPORT
  if (is_detach) return;
#endif
  skip_dut_nr_instr += nr_dut;
  while (nr_ref -- > 0) {
    ref_difftest_exec(1);
  }
}

void difftest_set_patch(void (*fn)(void *arg), void *arg) {
  patch_fn = fn;
  patch_arg = arg;
}

void init_difftest(char *ref_so_file, long img_size, int port) {
  assert(ref_so_file != NULL);

  void *handle;
  handle = dlopen(ref_so_file, RTLD_LAZY | MUXNDEF(CONFIG_CC_ASAN, RTLD_DEEPBIND, 0));
  assert(handle);

  ref_difftest_memcpy = dlsym(handle, "difftest_memcpy");
  assert(ref_difftest_memcpy);

  ref_difftest_regcpy = dlsym(handle, "difftest_regcpy");
  assert(ref_difftest_regcpy);

  ref_difftest_exec = dlsym(handle, "difftest_exec");
  assert(ref_difftest_exec);

  ref_difftest_raise_intr = dlsym(handle, "difftest_raise_intr");
  assert(ref_difftest_raise_intr);

  void (*ref_difftest_init)(int) = dlsym(handle, "difftest_init");
  assert(ref_difftest_init);

#ifdef CONFIG_DIFFTEST_REF_SPIKE
  ref_difftest_store_commit = dlsym(handle, "difftest_store_commit");
  assert(ref_difftest_store_commit);
#endif

  Log("Differential testing: \33[1;32m%s\33[0m", "ON");
  Log("The result of every instruction will be compared with %s. "
      "This will help you a lot for debugging, but also significantly reduce the performance. "
      "If it is not necessary, you can turn it off in include/common.h.", ref_so_file);

  ref_difftest_init(port);
  ref_difftest_memcpy(RESET_VECTOR, guest_to_host(RESET_VECTOR), img_size, DIFFTEST_TO_REF);
  ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
}

static void checkregs(CPU_state *ref, vaddr_t pc) {
  if (!isa_difftest_checkregs(ref, pc)) {
    isa_reg_display();
    IFDEF(CONFIG_IQUEUE, iqueue_dump());
    nemu_state.state = NEMU_ABORT;
    nemu_state.halt_pc = pc;
    longjmp_exec(NEMU_EXEC_END);
  }
}

void difftest_step(vaddr_t pc, vaddr_t npc) {
  CPU_state ref_r;

#ifndef __ICS_EXPORT
  if (is_detach) return;

#endif
  if (skip_dut_nr_instr > 0) {
    ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);
    if (ref_r.pc == npc) {
      skip_dut_nr_instr = 0;
      checkregs(&ref_r, npc);
      return;
    }
    skip_dut_nr_instr --;
    if (skip_dut_nr_instr == 0)
      panic("can not catch up with ref.pc = " FMT_WORD " at pc = " FMT_WORD " npc = " FMT_WORD, ref_r.pc, pc, npc);
    return;
  }

  if (is_skip_ref) {
    // Logti("is_skip_ref\n");
    // to skip the checking of an instruction, just copy the reg state to reference design
    ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
    is_skip_ref = false;
    return;
  }

  ref_difftest_exec(1);

  if (patch_fn) {
    patch_fn(patch_arg);
    patch_fn = NULL;
  }

  ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);
  // Log("run ref %lx, %lx, %ld", pc, ref_r.pc, cpu.v);
  checkregs(&ref_r, pc);
}
#ifndef __ICS_EXPORT
void difftest_detach() {
  is_detach = true;
}

void difftest_attach() {
  is_detach = false;
  is_skip_ref = false;
  skip_dut_nr_instr = 0;

  isa_difftest_attach();
}
#endif

#else
void init_difftest(char *ref_so_file, long img_size, int port) { }
#endif
