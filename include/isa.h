#ifndef __ISA_H__
#define __ISA_H__

#include _ISA_H_

// monitor
extern char isa_logo[];
void init_isa();

// reg
extern CPU_state cpu;
void isa_reg_display();
word_t isa_reg_str2val(const char *name, bool *success);

// exec
vaddr_t isa_exec_once();

// memory
uint8_t  isa_vaddr_read8 (vaddr_t addr);
uint16_t isa_vaddr_read16(vaddr_t addr);
uint32_t isa_vaddr_read32(vaddr_t addr);
uint64_t isa_vaddr_read64(vaddr_t addr);
void isa_vaddr_write8 (vaddr_t addr, uint8_t  data);
void isa_vaddr_write16(vaddr_t addr, uint16_t data);
void isa_vaddr_write32(vaddr_t addr, uint32_t data);
void isa_vaddr_write64(vaddr_t addr, uint64_t data);

static inline uint64_t vaddr_read(vaddr_t addr, int len) {
  switch (len) {
    case 1: return isa_vaddr_read8 (addr);
    case 2: return isa_vaddr_read16(addr);
    case 4: return isa_vaddr_read32(addr);
#ifdef ISA64
    case 8: return isa_vaddr_read64(addr);
#endif
    default: assert(0);
  }
}

static inline void vaddr_write(vaddr_t addr, uint64_t data, int len) {
  switch (len) {
    case 1: isa_vaddr_write8 (addr, data); break;
    case 2: isa_vaddr_write16(addr, data); break;
    case 4: isa_vaddr_write32(addr, data); break;
#ifdef ISA64
    case 8: isa_vaddr_write64(addr, data); break;
#endif
    default: assert(0);
  }
}

// difftest
  // for dut
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc);
void isa_difftest_attach();

  // for ref
void isa_difftest_getregs(void *r);
void isa_difftest_setregs(const void *r);
void isa_difftest_raise_intr(word_t NO);

#endif
