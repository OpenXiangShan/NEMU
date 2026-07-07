#ifndef __CPU_INSTR_STAT_H__
#define __CPU_INSTR_STAT_H__

#include <stddef.h>
#include <stdint.h>

typedef enum {
  INSTR_STAT_ALU,
  INSTR_STAT_LOAD,
  INSTR_STAT_STORE,
  INSTR_STAT_BRANCH,
  INSTR_STAT_JUMP,
  INSTR_STAT_CSR_SYSTEM,
  INSTR_STAT_FP,
  INSTR_STAT_VECTOR,
  INSTR_STAT_AMO,
  INSTR_STAT_FENCE,
  INSTR_STAT_OTHER,
  INSTR_STAT_NR,
} InstrStatCategory;

const char *instr_stat_category_name(InstrStatCategory category);
void instr_stat_count(int exec_id);
uint64_t instr_stat_get(InstrStatCategory category);
void instr_stat_reset(void);
int instr_stat_format(char *buf, size_t size);

#endif
