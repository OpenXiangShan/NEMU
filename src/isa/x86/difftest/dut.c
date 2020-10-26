#include <isa.h>
#include <monitor/difftest.h>
#include "../local-include/reg.h"
#include "difftest.h"

#ifndef __ICS_EXPORT
#include <memory/paddr.h>

#ifdef ENABLE_DIFFTEST_INSTR_QUEUE
#define INSTR_QUEUE_SIZE (1 << 15)
static uint32_t q_idx = 0;
struct {
  vaddr_t pc;
  uint8_t instr[20];
  uint8_t instr_len;
} instr_queue[INSTR_QUEUE_SIZE];

void commit_instr(vaddr_t thispc, uint8_t *instr_buf, uint8_t instr_len) {
  instr_queue[q_idx].pc = thispc;
  instr_queue[q_idx].instr_len = instr_len;
  assert(instr_len < 20);
  memcpy(instr_queue[q_idx].instr, instr_buf, instr_len);
  q_idx = (q_idx + 1) % INSTR_QUEUE_SIZE;
}

void dump_instr_queue() {
  int i;
  int victim_idx = (q_idx - 1) % INSTR_QUEUE_SIZE;
  printf("======== instruction queue =========\n");
  for (i = 0; i < INSTR_QUEUE_SIZE; i ++) {
    printf("%5s 0x%08x: ", (i == victim_idx ? "-->" : ""), instr_queue[i].pc);
    int j;
    for (j = 0; j < instr_queue[i].instr_len; j ++) {
      printf("%02x ", instr_queue[i].instr[j]);
    }
    printf("\n");
  }
  printf("======== instruction queue end =========\n");
}
#else
#define dump_instr_queue()
#endif

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  if (memcmp(&cpu, ref_r, DIFFTEST_REG_SIZE)) {
    int i;
    for (i = 0; i < sizeof(cpu.gpr) / sizeof(cpu.gpr[0]); i ++) {
      difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._32, cpu.gpr[i]._32);
    }
    difftest_check_reg("pc", pc, ref_r->pc, cpu.pc);
    dump_instr_queue();
    return false;
  }
  return true;
}

void isa_difftest_attach() {
  // first copy the image
  ref_difftest_memcpy_from_dut(0, guest_to_host(0), PMEM_SIZE);

  // then set some special registers
  uint8_t code[] = {
    // we put this code at 0x7e00
    0xb8, 0x00, 0x00, 0x00, 0x00,    // mov $0x0, %eax
    0x0f, 0x22, 0xd8,                // mov %eax, %cr3
    0xb8, 0x00, 0x00, 0x00, 0x00,    // mov $0x0, %eax
    0x0f, 0x22, 0xc0,                // mov %eax, %cr0
    0x0f, 0x01, 0x1d, 0x40, 0x7e, 0x00, 0x00,     // lidtl (0x7e40)
  };
  uint8_t idtdesc[6];

  *(uint32_t *)(code + 1) = cpu.cr3.val;
  *(uint32_t *)(code + 9) = cpu.cr0.val;

  idtdesc[0] = cpu.idtr.limit & 0xff;
  idtdesc[1] = cpu.idtr.limit >> 8;
  *(uint32_t *)(idtdesc + 2) = cpu.idtr.base;

  assert(sizeof(code) < 0x40);
  ref_difftest_memcpy_from_dut(0x7e00, code, sizeof(code));
  ref_difftest_memcpy_from_dut(0x7e40, idtdesc, sizeof(idtdesc));

  CPU_state r = cpu;
  r.pc = 0x7e00;
  ref_difftest_setregs(&r);
  ref_difftest_exec(5);

  ref_difftest_setregs(&cpu);
}
#else
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  return false;
}

void isa_difftest_attach() {
}
#endif
