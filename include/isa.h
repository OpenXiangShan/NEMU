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
void isa_exec(vaddr_t *pc);

// memory
word_t isa_vaddr_read(vaddr_t addr, int len);
void isa_vaddr_write(vaddr_t addr, word_t data, int len);

// difftest
void isa_difftest_getregs(void *r);
void isa_difftest_setregs(const void *r);

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc);
void isa_difftest_attach();

#endif
