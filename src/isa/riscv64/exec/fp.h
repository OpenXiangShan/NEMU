#include "../local-include/softfloat/softfloat.h"
#include "../local-include/softfloat/specialize.h"
#include "../local-include/softfloat/internals.h"

#include "../local-include/intr.h"


#define BOX_MASK 0xFFFFFFFF00000000
#define F32_SIGN ((uint32_t)1 << 31)
#define F64_SIGN ((uint64_t)1 << 63)

static inline bool assert_ex_ii(DecodeExecState *s, bool cond){
    if(!cond){
        raise_intr(s, EX_II, cpu.pc);
        return false;
    }
    return true;
}

static inline bool check_fs(DecodeExecState *s) {
    return assert_ex_ii(s, mstatus->fs != 0);
}

static inline bool get_rm(DecodeExecState *s, int *rm) {
    if(s->isa.instr.fp.rm == 7){
        *rm = frm->val;
        return true;
    }
    *rm = s->isa.instr.fp.rm;
    return assert_ex_ii(s, s->isa.instr.fp.rm <= 4);
}

static inline rtlreg_t unbox(rtlreg_t r) {
    if((r & BOX_MASK) == BOX_MASK){
        return r & ~BOX_MASK;
    } else {
        return defaultNaNF32UI;
    }
}

static inline rtlreg_t box(rtlreg_t r){
    return BOX_MASK | r;
}


static inline rtlreg_t fsgnj32(rtlreg_t a, rtlreg_t b, bool n, bool x){
    return (unbox(a) & ~F32_SIGN) | ((( x ? unbox(a) : n ? F32_SIGN : 0) ^ unbox(b)) & F32_SIGN);
}

static inline rtlreg_t fsgnj64(rtlreg_t a, rtlreg_t b, bool n, bool x){
    return (a & ~F64_SIGN) | ((( x ? a : n ? F64_SIGN : 0) ^ b) & F64_SIGN);
}

static inline void sfpr(rtlreg_t *pdst, rtlreg_t *psrc, int width) {
    if(width == 4) *pdst = box(*psrc);
    else if( width==8 ) *pdst = *psrc;
    mstatus->sd = 1;
    mstatus->fs = 3;
    return;
}

static inline make_EHelper(fp_ld) {
    if(!check_fs(s)) return;
    if(s->width == 8) rtl_lm(s, s0, dsrc1, id_src2->imm, s->width);
    else if(s->width == 4) rtl_lm(s, s0, dsrc1, id_src2->imm, s->width);
    sfpr(ddest, s0, s->width);
    print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
    switch (s->width) {
        case 8: print_asm_template2(fld); break;
        case 4: print_asm_template2(flw); break;
        default: assert(0);
    }
}

static inline make_EHelper(fp_st) {
    *s0 = s->width == 4 ? box(*ddest) : *ddest;
    rtl_sm(s, dsrc1, id_src2->imm, s0, s->width);

    print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
    switch (s->width) {
        case 8: print_asm_template2(fsd); break;
        case 4: print_asm_template2(fsw); break;
        default: assert(0);
    }
}



static inline make_EHelper(fsgnj) {
    if(!check_fs(s)) return;
    int rm = s->isa.instr.fp.rm;
    bool is_neg = rm == 1;
    bool is_xor = rm == 2;
    if(!assert_ex_ii(s, rm==0 || is_neg || is_xor)) return;

    *s0 = s->width == 4 ? fsgnj32(*dsrc1, *dsrc2, is_neg, is_xor) : fsgnj64(*dsrc1, *dsrc2, is_neg, is_xor);
    sfpr(ddest, s0, s->width);

    switch (rm)
    {
    case 0: // sgnj
        if(s->width == 4) print_asm_template3(fsgnj.s);
        else print_asm_template3(fsgnj.d);
        break;
    case 1: // sgnjn
        if(s->width == 4) print_asm_template3(fsgnjn.s);
        else print_asm_template3(fsgnjn.d);
        break;
    case 2: // sgnjx
        if(s->width == 4) print_asm_template3(fsgnjx.s);
        else print_asm_template3(fsgnjx.d);
        break;
    default:
        assert(0);
        break;
    }
}

