#ifndef __CPU_EXEC_H__
#define __CPU_EXEC_H__

#include <cpu/decode.h>

#ifdef CONFIG_PERF_OPT
#define finish_label exec_finish
#define def_label(l) l:
#define def_EHelper(name) \
  s ++; \
  goto finish_label; /* this is for the previous def_EHelper() */ \
  def_label(concat(exec_, name))
#define def_finish() def_label(finish_label)
#else
#define def_EHelper(name) static inline void concat(exec_, name) (Decode *s)
#endif

#endif
