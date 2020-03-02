#ifndef __ISA_H__
#define __ISA_H__

#include <common.h>
#include _ISA_H_

// monitor
extern const uint8_t isa_default_img[];
extern const long isa_default_img_size;
extern char isa_logo[];

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

// difftest
void isa_difftest_getregs(void *r);
void isa_difftest_setregs(const void *r);

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc);
void isa_difftest_attach();

#endif
