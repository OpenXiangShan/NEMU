#include "../softfloat/softfloat.h"
#include "../softfloat/specialize.h"
#include "../softfloat/internals.h"

#include "../local-include/intr.h"


#define BOX_MASK 0xFFFFFFFF00000000
#define F32_SIGN ((uint32_t)1 << 31)
#define F64_SIGN ((uint64_t)1 << 63)

// ------------- dasm -------------

#define print_asm_fpu_template_3(inst, fmt) \
  print_asm(str(inst) "." str(fmt) "%c %s,%s,%s", \
    suffix_char(id_dest->width), id_src1->str, id_src2->str, id_dest->str)

#define print_asm_fpu_template_4(inst, fmt) \
  print_asm(str(inst) "." str(fmt) "%c %s,%s,%s,%s", \
    suffix_char(id_dest->width), id_src1->str, id_src2->str, fpreg_name(s->isa.instr.fp.funct5, 4), id_dest->str)

// ------------- helper functions -------------

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

static inline float32_t rtlToF32(rtlreg_t r){
    float32_t f;
    f.v = unbox(r);
    return f;
}

static inline float64_t rtlToF64(rtlreg_t r){
    float64_t f;
    f.v = r;
    return f;
}

inline rtlreg_t neg64(rtlreg_t a){
  return a ^ F64_SIGN;
}

inline rtlreg_t neg32(rtlreg_t a){
  return ((uint64_t)-1 << 32) | (a ^ F32_SIGN);
}

float32_t f32_min(float32_t a, float32_t b){
  bool less = f32_lt_quiet(a, b) || (f32_eq(a, b) && (a.v & F32_SIGN));
  if(isNaNF32UI(a.v) && isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return(less || isNaNF32UI(b.v) ? a : b);
}

float64_t f64_min(float64_t a, float64_t b){
  bool less = f64_lt_quiet(a, b) || (f64_eq(a, b) && (a.v & F64_SIGN));
  if(isNaNF64UI(a.v) && isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return(less || isNaNF64UI(b.v) ? a : b);
}

float32_t f32_max(float32_t a, float32_t b){
  bool greater = f32_lt_quiet(b, a) || (f32_eq(b, a) && (b.v & F32_SIGN));
  if(isNaNF32UI(a.v) && isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return(greater || isNaNF32UI(b.v) ? a : b);
}

float64_t f64_max(float64_t a, float64_t b){
  bool greater = f64_lt_quiet(b, a) || (f64_eq(b, a) && (b.v & F64_SIGN));
  if(isNaNF64UI(a.v) && isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return(greater || isNaNF64UI(b.v) ? a : b);
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

static inline void writeFflags(DecodeExecState *s) {
    if(softfloat_exceptionFlags){
        mstatus->sd = 1;
        mstatus->fs = 3;
        *s1 = fflags->val | softfloat_exceptionFlags;
        csr_write(1, s1);
        softfloat_exceptionFlags = 0;
    }
}


// ------------- EHelpers -------------

static inline make_EHelper(fp_ld) {
    if(!check_fs(s)) return;
    if(s->width == 8) rtl_lm(s, s0, dsrc1, id_src2->imm, s->width);
    else if(s->width == 4) rtl_lm(s, s0, dsrc1, id_src2->imm, s->width);
    if (!isa_has_mem_exception()) sfpr(ddest, s0, s->width);

    print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
    switch (s->width) {
        case 8: print_asm_template2(fld); break;
        case 4: print_asm_template2(flw); break;
        default: assert(0);
    }
}

static inline make_EHelper(fp_st) {
    *s0 = s->width == 4 ? unbox(*ddest) : *ddest;
    rtl_sm(s, dsrc1, id_src2->imm, s0, s->width);

    print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(id_src1->reg, 4));
    switch (s->width) {
        case 8: print_asm_template2(fsd); break;
        case 4: print_asm_template2(fsw); break;
        default: assert(0);
    }
}



// a macro to build exec_fadd/fsub/fmul/fmdiv/fmin/fmax
#define BUILD_EXEC_F(x) \
  static inline make_EHelper(concat(f, x)) { \
    if(!check_fs(s)) return; \
    int rm; \
    if(!get_rm(s, &rm)) return; \
    softfloat_roundingMode = rm; \
    switch (s->width) \
    { \
    case 8: \
      *s0 = concat(f64_, x)(rtlToF64(*dsrc1), rtlToF64(*dsrc2)).v; \
      print_asm_fpu_template_3(concat(f, x), d); \
      break; \
    case 4: \
      *s0 = concat(f32_, x)(rtlToF32(*dsrc1), rtlToF32(*dsrc2)).v; \
      print_asm_fpu_template_3(concat(f, x), s); \
      break; \
    default: \
      assert(0); \
    } \
    sfpr(ddest, s0, s->width); \
    writeFflags(s); \
  }

BUILD_EXEC_F(add);
BUILD_EXEC_F(sub);
BUILD_EXEC_F(mul);
BUILD_EXEC_F(div);
BUILD_EXEC_F(min);
BUILD_EXEC_F(max);

static inline make_EHelper(fmin_fmax){
    if(s->isa.instr.fp.rm == 0)
        exec_fmin(s);
    else 
        exec_fmax(s);
}

// macros to build fma instructions
#define NEG(w, src) concat(neg, w)(src)

#define fnmsub(w, src1, src2, src3) \
  concat3(f, w, _mulAdd)( \
    concat(rtlToF, w)(NEG(w, src1)), \
    concat(rtlToF, w)(src2), \
    concat(rtlToF, w)(src3) \
  ).v
#define fmsub(w, src1, src2, src3) \
  concat3(f, w, _mulAdd)( \
    concat(rtlToF, w)(src1), \
    concat(rtlToF, w)(src2), \
    concat(rtlToF ,w)(NEG(w, src3)) \
  ).v
#define fmadd(w, src1, src2, src3) \
  concat3(f, w, _mulAdd)( \
    concat(rtlToF, w)(src1), \
    concat(rtlToF, w)(src2), \
    concat(rtlToF, w)(src3) \
  ).v
#define fnmadd(w, src1, src2, src3) \
  concat3(f, w, _mulAdd)( \
    concat(rtlToF, w)(NEG(w, src1)), \
    concat(rtlToF, w)(src2), \
    concat(rtlToF, w)(NEG(w, src3)) \
  ).v

#define BUILD_EXEC_FM(func) \
  static inline make_EHelper(func){ \
    if(!check_fs(s)) return; \
    int rm; \
    if(!get_rm(s, &rm)) return; \
    softfloat_roundingMode = rm; \
    *s1 = fpreg_l(s->isa.instr.fp.funct5); \
    switch (s->width) \
    { \
    case 8: \
      *s0 = func(64, *dsrc1, *dsrc2, *s1); \
      print_asm_fpu_template_4(func, d); \
      break; \
    case 4: \
      *s0 = func(32, *dsrc1, *dsrc2, *s1); \
      print_asm_fpu_template_4(func, s); \
      break; \
    default: \
      assert(0); \
    } \
    sfpr(ddest, s0, s->width); \
    writeFflags(s); \
  }

BUILD_EXEC_FM(fmadd);
BUILD_EXEC_FM(fmsub);
BUILD_EXEC_FM(fnmsub);
BUILD_EXEC_FM(fnmadd);

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

// fmv.x.d/fmv.x.w and fclass 
static inline make_EHelper(fmv_F_to_G) {
    if(!check_fs(s)) return;
    int rm = s->isa.instr.fp.rm;
    if(!assert_ex_ii(s, rm==0 || rm==1)) return;

    if(rm == 0){
        // fmv fpr to gpr
        rtl_sext(s, s0, dsrc1, s->width);
        if(s->width == 4) print_asm_template2(fmv.x.w);
        else print_asm_template2(fmv.x.d);
    } else {
        // fclass
        if(s->width == 4){
            *s0 = f32_classify(rtlToF32(*dsrc1));
            print_asm_template2(fclass.s);
        } else {
            *s0 = f64_classify(rtlToF64(*dsrc1));
            print_asm_template2(fclass.d);
        }
    }
    rtl_sr(s, s->dest.reg, s0, 8);
}

static inline make_EHelper(fmv_G_to_F){
    if(!check_fs(s)) return;
    sfpr(ddest, dsrc1, s->width);

    switch (s->width){
        case 4:
            print_asm_template2(fmv.w.x); break;
        case 8:
            print_asm_template2(fmv.d.x); break;
        default:
            assert(0);
            break;
    }
}

static inline make_EHelper(fcmp){
    if(!check_fs(s)) return;
    int rm = s->isa.instr.fp.rm;
    if(!assert_ex_ii(s, rm==0 || rm==1 || rm==2)) return;

    static bool (* f_table[3])(float32_t, float32_t) = {
        f32_le, f32_lt, f32_eq
    };
    static bool (* d_table[3])(float64_t, float64_t) = {
        f64_le, f64_lt, f64_eq
    };

    if(s->width == 4) *s0 = (f_table[rm])(rtlToF32(*dsrc1), rtlToF32(*dsrc2));
    else if(s->width == 8) *s0 = (d_table[rm])(rtlToF64(*dsrc1), rtlToF64(*dsrc2));

    rtl_sr(s, s->dest.reg, s0, s->width);
    writeFflags(s);

    switch (rm)
    {
    case 0: // fle
        if(s->width == 4){
            print_asm_template3(fle.s);  
        }
        else{
            print_asm_template3(fle.d);  
        }
        break;
    case 1: // flt
        if(s->width == 4){
            print_asm_template3(flt.s);  
        }
        else{
            print_asm_template3(flt.d);  
        }
        break;
    case 2: // feq
        if(s->width == 4){
            print_asm_template3(feq.s);  
        }
        else{
            print_asm_template3(feq.d);  
        }
        break;
    default:
        assert(0);
        break;
    }
}

static inline make_EHelper(fsqrt) {
    if(!check_fs(s)) return;
    int rm;
    if(!get_rm(s, &rm)) return;
    
    softfloat_roundingMode = rm;

    if(s->width == 4){
        *s0 = f32_sqrt(rtlToF32(*dsrc1)).v;
    } else if(s->width == 8){
        *s0 = f64_sqrt(rtlToF64(*dsrc1)).v;
    }
    sfpr(ddest, s0, s->width);
    writeFflags(s);

    switch (s->width)
    {
    case 4:
        print_asm_template2(fsqrt.s); break;
    case 8:
        print_asm_template2(fsqrt.d); break;
    default:
        assert(0);
        break;
    }
}

static inline make_EHelper(fcvt_F_to_G){
    if(!check_fs(s)) return;
    int rm;
    if(!get_rm(s, &rm)) return;
    
    softfloat_roundingMode = rm;

    switch(s->isa.instr.fp.rs2){
        case 0: // fcvt.w.[s/d]
            if(s->width == 4){
                *s0 = f32_to_i32(rtlToF32(*dsrc1), rm, true);
                print_asm_template2(fcvt.w.s);
            } else {
                *s0 = f64_to_i32(rtlToF64(*dsrc1), rm, true);
                print_asm_template2(fcvt.w.d);
            }
            rtl_sext(s, s0, s0, 4);
            break;
        case 1: // fcvt.wu.[s/d]
            if(s->width == 4){
                *s0 = f32_to_ui32(rtlToF32(*dsrc1), rm, true);
                print_asm_template2(fcvt.uw.s);
            } else {
                *s0 = f64_to_ui32(rtlToF64(*dsrc1), rm, true);
                print_asm_template2(fcvt.uw.d);
            }
            rtl_sext(s, s0, s0, 4);
            break;
        case 2: // fcvt.l.[s/d]
            if(s->width == 4){
                *s0 = f32_to_i64(rtlToF32(*dsrc1), rm, true);
                print_asm_template2(fcvt.l.s);
            } else {
                *s0 = f64_to_i64(rtlToF64(*dsrc1), rm, true);
                print_asm_template2(fcvt.l.d);
            }
            break;
        case 3: // fcvt.ul.[s/d]
            if(s->width == 4){
                *s0 = f32_to_ui64(rtlToF32(*dsrc1), rm, true);
                print_asm_template2(fcvt.ul.s);
            } else {
                *s0 = f64_to_ui64(rtlToF64(*dsrc1), rm, true);
                print_asm_template2(fcvt.ul.d);
            }
            break;
    }
    rtl_sr(s, s->dest.reg, s0, 8);
    writeFflags(s);
}

static inline make_EHelper(fcvt_G_to_F){
    if(!check_fs(s)) return;
    int rm;
    if(!get_rm(s, &rm)) return;
    
    softfloat_roundingMode = rm;

    switch(s->isa.instr.fp.rs2){
        case 0: // fcvt.[s/d].w
            if(s->width == 4){
                *s0 = i32_to_f32(*dsrc1).v;
                print_asm_template2(fcvt.s.w);
            } else {
                *s0 = i32_to_f64(*dsrc1).v;
                print_asm_template2(fcvt.d.w);
            }
            break;
        case 1: // fcvt.[s/d].wu
            if(s->width == 4){
                *s0 = ui32_to_f32(*dsrc1).v;
                print_asm_template2(fcvt.s.uw);
            } else {
                *s0 = ui32_to_f64(*dsrc1).v;
                print_asm_template2(fcvt.d.uw);
            }
            break;
        case 2: // fcvt.[s/d].l
            if(s->width == 4){
                *s0 = i64_to_f32(*dsrc1).v;
                print_asm_template2(fcvt.s.l);
            } else {
                *s0 = i64_to_f64(*dsrc1).v;
                print_asm_template2(fcvt.d.l);
            }
            break;
        case 3: // fcvt.[s/d].ul
            if(s->width == 4){
                *s0 = ui64_to_f32(*dsrc1).v;
                print_asm_template2(fcvt.s.ul);
            } else {
                *s0 = ui64_to_f64(*dsrc1).v;
                print_asm_template2(fcvt.d.ul);
            }
            break;
    }
    sfpr(ddest, s0, s->width);
    writeFflags(s);
}

static inline make_EHelper(fcvt_F_to_F) {
    if(!check_fs(s)) return;
    int rm;
    if(!get_rm(s, &rm)) return;
    
    softfloat_roundingMode = rm;

    if(s->isa.instr.fp.fmt == 0 && s->isa.instr.fp.rs2 == 1){
        // fcvt.s.d
        *s0 = f64_to_f32(rtlToF64(*dsrc1)).v;
        print_asm_template2(fcvt.s.d);
    } 
    else if(s->isa.instr.fp.fmt == 1 && s->isa.instr.fp.rs2 == 0){
        // fcvt.d.s
        *s0 = f32_to_f64(rtlToF32(*dsrc1)).v;
        print_asm_template2(fcvt.d.s);
    } else {
        assert(0);
    }

    sfpr(ddest, s0, s->width);
    writeFflags(s);
}
