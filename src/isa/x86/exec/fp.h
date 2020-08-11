#include "cc.h"
#define dfdest (&id_dest->fval)
#define dfsrc1 (&id_src1->fval)
#define dfsrc2 (&id_src2->fval)

// ------------- dasm -------------

#define print_asm_fpu_template(instr) \
  print_asm(str(instr) "%d %s,%s", s->isa.fpu_MF, id_src1->str, id_dest->str)


// ------------- EHelpers -------------

// a macro to build exec_fadd/fsub/fmul/fmdiv/fmin/fmax
#define BUILD_EXEC_F(x) \
static inline make_EHelper(concat(f, x)) { \
    concat(rtl_f, x)(dfdest, dfdest, dfsrc1); \
    operand_fwrite(s, id_dest, dfdest); \
    print_asm_fpu_template(concat(f, x)); \
}

BUILD_EXEC_F(add);
BUILD_EXEC_F(sub);
BUILD_EXEC_F(mul);
BUILD_EXEC_F(div);
