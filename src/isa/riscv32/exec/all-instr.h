#define update_gpc(npc) (cpu.pc = (npc)) // global pc
#define update_lpc(npc) (lpc = (npc)) // local pc

#define rtl_j(s, target) update_lpc(target)
#define rtl_jr(s, target) update_lpc(*(target))
#define rtl_jrelop(s, relop, src1, src2, target) \
  do { if (interpret_relop(relop, *src1, *src2)) lpc = target; } while (0)

#undef def_EHelper
#define finish_label exec_finish
#define def_label(l) l:
#define def_EHelper(name) \
  goto finish_label; /* this is for the previous def_EHelper() */ \
  def_label(name)

#define def_start()
#define def_finish() def_label(finish_label)

#ifndef DEBUG
#undef id_dest
#undef id_src1
#undef id_src2
#define id_dest (&ldest)
#define id_src1 (&lsrc1)
#define id_src2 (&lsrc2)
#endif

#include "special.h"
#include "compute.h"
#include "control.h"
#include "ldst.h"
#include "muldiv.h"
#include "system.h"
