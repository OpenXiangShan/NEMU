#define INSTR_RS3(s) BITS(s->extraInfo->isa.instr.val, 31, 27)
#define INSTR_FP_RM(s) BITS(s->extraInfo->isa.instr.val, 14, 12)

#include "../instr/pseudo.h"
#include "../instr/rvi.h"
#include "../instr/rvc.h"
#include "../instr/rvm.h"
#include "../instr/rvf.h"
#include "../instr/rvd.h"
#include "../instr/rva.h"
#include "../instr/priv.h"
#include "../instr/special.h"
