#ifndef __CPU_EXEC_H__
#define __CPU_EXEC_H__

#include <cpu/decode.h>
#include <memory/vaddr.h>

#ifdef CONFIG_PERF_OPT
#define finish_label exec_finish
#define def_label(l) l:
#define def_EHelper(name) \
  goto finish_label; /* this is for the previous def_EHelper() */ \
  def_label(name)
#define def_finish() def_label(finish_label)
#else
#define def_EHelper(name) void concat(exec_, name) (DecodeExecState *s)
#endif

#if 0
#define IDEXW(idx, id, ex, w) CASE_ENTRY(idx, concat(decode_, id), concat(exec_, ex), w)
#define IDEX(idx, id, ex)     IDEXW(idx, id, ex, 0)
#define EXW(idx, ex, w)       IDEXW(idx, empty, ex, w)
#define EX(idx, ex)           EXW(idx, ex, 0)
#define EMPTY(idx)            EX(idx, inv)

// set_width() is defined in src/isa/$isa/exec/exec.c
#define CASE_ENTRY(idx, id, ex, w) case idx: set_width(s, w); id(s); ex(s); break;
#endif

static inline uint32_t instr_fetch(vaddr_t *pc, int len) {
  uint32_t instr = vaddr_ifetch(*pc, len);
#ifdef ENABLE_DIFFTEST_INSTR_QUEUE
  extern void add_instr(uint8_t *instr, int len);
  add_instr((void *)&instr, len);
#endif
#ifdef CONFIG_DEBUG
  uint8_t *p_instr = (void *)&instr;
  int i;
  for (i = 0; i < len; i ++) {
    int l = strlen(log_bytebuf);
    snprintf(log_bytebuf + l, sizeof(log_bytebuf) - l, "%02x ", p_instr[i]);
  }
#endif
  (*pc) += len;
  return instr;
}

static inline void update_pc(DecodeExecState *s) {
  //cpu.pc = s->npc;
}
#endif
