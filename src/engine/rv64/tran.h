#ifndef __TRAN_H__
#define __TRAN_H__

extern int tran_next_pc;
enum { NEXT_PC_SEQ, NEXT_PC_JMP, NEXT_PC_BRANCH };

#define BBL_MAX_SIZE (16 * 1024)
#define RV64_EXEC_PC BBL_MAX_SIZE // skip bbl

#endif
