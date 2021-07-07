#ifndef __FP_INCLUDE_H__
#define __FP_INCLUDE_H__

#define ftop_pop()  (cpu.ftop = (cpu.ftop + 1) & 0x7)
#define ftop_pop2() (cpu.ftop = (cpu.ftop + 2) & 0x7)
#define ftop_push() (cpu.ftop = (cpu.ftop - 1) & 0x7)

#define dfdest (id_dest->pfreg)
#define dfsrc1 (id_src1->pfreg)
#define dfsrc2 (id_src2->pfreg)

#endif
