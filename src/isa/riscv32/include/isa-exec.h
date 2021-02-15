#ifdef CONFIG_PERF_OPT
#define update_lpc(npc) (lpc = (npc)) // local pc

#define rtl_j(s, target) update_lpc(target)
#define rtl_jr(s, target) update_lpc(*(target))
#define rtl_jrelop(s, relop, src1, src2, target) \
  do { if (interpret_relop(relop, *src1, *src2)) lpc = target; } while (0)

#undef id_dest
#undef id_src1
#undef id_src2
#define id_dest (&ldest)
#define id_src1 (&lsrc1)
#define id_src2 (&lsrc2)
#endif

#include "../instr/special.h"
#include "../instr/compute.h"
#include "../instr/control.h"
#include "../instr/ldst.h"
#include "../instr/muldiv.h"
#include "../instr/system.h"
