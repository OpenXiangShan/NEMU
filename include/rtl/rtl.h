#ifndef __RTL_RTL_H__
#define __RTL_RTL_H__

#include <cpu/decode.h>

extern const rtlreg_t rzero;
extern rtlreg_t tmp_reg[4];

#define dsrc1 (id_src1->preg)
#define dsrc2 (id_src2->preg)
#define ddest (id_dest->preg)
#define s0    (&tmp_reg[0])
#define s1    (&tmp_reg[1])
#define s2    (&tmp_reg[2])
#define t0    (&tmp_reg[3])
#define rz (&rzero)

#define def_rtl(name, ...) void concat(rtl_, name)(Decode *s, __VA_ARGS__)

// relation operation
enum {
  //            +-- unsign
  //            |   +-- sign
  //            |   |   +-- equal
  //            |   |   |   +-- invert
  //            |   |   |   |
  RELOP_FALSE = 0 | 0 | 0 | 0,
  RELOP_TRUE  = 0 | 0 | 0 | 1,
  RELOP_EQ    = 0 | 0 | 2 | 0,
  RELOP_NE    = 0 | 0 | 2 | 1,

  RELOP_LT    = 0 | 4 | 0 | 0,
  RELOP_LE    = 0 | 4 | 2 | 0,
  RELOP_GT    = 0 | 4 | 2 | 1,
  RELOP_GE    = 0 | 4 | 0 | 1,

  RELOP_LTU   = 8 | 0 | 0 | 0,
  RELOP_LEU   = 8 | 0 | 2 | 0,
  RELOP_GTU   = 8 | 0 | 2 | 1,
  RELOP_GEU   = 8 | 0 | 0 | 1,
};

enum {
  HOSTCALL_EXIT,  // handling nemu_trap
  HOSTCALL_INV,   // invalid opcode
  HOSTCALL_PIO,   // port I/O
#ifndef __ICS_EXPORT
  HOSTCALL_CSR,   // system registers / control status registers
  HOSTCALL_TRAP_THIS,  // trap by interrupts/exceptions, save this pc
  HOSTCALL_TRAP_NEXT,  // trap by interrupts/exceptions, save next pc
  HOSTCALL_PRIV,  // privilige instructions
#endif
};

def_rtl(hostcall, uint32_t id, rtlreg_t *dest, const rtlreg_t *src1,
    const rtlreg_t *src2, word_t imm);

#include <rtl-basic.h>
#include <rtl/pseudo.h>

#ifndef __ICS_EXPORT
#include <rtl/fp.h>

def_rtl(flm, fpreg_t *dest, const rtlreg_t *addr, sword_t offset, int len, int mmu_mode);
def_rtl(fsm, const fpreg_t *src1, const rtlreg_t *addr, sword_t offset, int len, int mmu_mode);

#define def_rtl_fp_unary_prototype(name) \
  def_rtl(name, fpreg_t *dest, const fpreg_t *src1)
#define def_rtl_fp_binary_prototype(name) \
  def_rtl(name, fpreg_t *dest, const fpreg_t *src1, const fpreg_t *src2)
#define def_rtl_fp_ternary_prototype(name) def_rtl_fp_binary_prototype(name)
#define def_rtl_fp_cmp_prototype(name) \
  def_rtl(name, rtlreg_t *dest, const fpreg_t *src1, const fpreg_t *src2)
#define def_rtl_i2f_prototype(name) \
  def_rtl(name, fpreg_t *dest, const rtlreg_t *src1)
#define def_rtl_i642f_prototype(name) \
  def_rtl(name, fpreg_t *dest, const fpreg_t *src1)
#define def_rtl_f2i_prototype(name) \
  def_rtl(name, rtlreg_t *dest, const fpreg_t *src1)
#define def_rtl_f2i64_prototype(name) \
  def_rtl(name, fpreg_t *dest, const fpreg_t *src1)

def_rtl_fp_binary_prototype(fadds);
def_rtl_fp_binary_prototype(fsubs);
def_rtl_fp_binary_prototype(fmuls);
def_rtl_fp_binary_prototype(fdivs);
def_rtl_fp_binary_prototype(fmins);
def_rtl_fp_binary_prototype(fmaxs);
def_rtl_fp_unary_prototype(fsqrts);
def_rtl_fp_ternary_prototype(fmadds);
def_rtl_fp_cmp_prototype(fles);
def_rtl_fp_cmp_prototype(flts);
def_rtl_fp_cmp_prototype(feqs);
def_rtl_i2f_prototype(fcvt_i32_to_f32);
def_rtl_i2f_prototype(fcvt_u32_to_f32);
def_rtl_i642f_prototype(fcvt_i64_to_f32);
def_rtl_i642f_prototype(fcvt_u64_to_f32);
def_rtl_f2i_prototype(fcvt_f32_to_i32);
def_rtl_f2i_prototype(fcvt_f32_to_u32);
def_rtl_f2i64_prototype(fcvt_f32_to_i64);
def_rtl_f2i64_prototype(fcvt_f32_to_u64);

def_rtl_fp_binary_prototype(faddd);
def_rtl_fp_binary_prototype(fsubd);
def_rtl_fp_binary_prototype(fmuld);
def_rtl_fp_binary_prototype(fdivd);
def_rtl_fp_binary_prototype(fmind);
def_rtl_fp_binary_prototype(fmaxd);
def_rtl_fp_unary_prototype(fsqrtd);
def_rtl_fp_ternary_prototype(fmaddd);
def_rtl_fp_cmp_prototype(fled);
def_rtl_fp_cmp_prototype(fltd);
def_rtl_fp_cmp_prototype(feqd);
def_rtl_i2f_prototype(fcvt_i32_to_f64);
def_rtl_i2f_prototype(fcvt_u32_to_f64);
def_rtl_i642f_prototype(fcvt_i64_to_f64);
def_rtl_i642f_prototype(fcvt_u64_to_f64);
def_rtl_f2i_prototype(fcvt_f64_to_i32);
def_rtl_f2i_prototype(fcvt_f64_to_u32);
def_rtl_f2i64_prototype(fcvt_f64_to_i64);
def_rtl_f2i64_prototype(fcvt_f64_to_u64);
def_rtl_fp_unary_prototype(fcvt_f32_to_f64);
def_rtl_fp_unary_prototype(fcvt_f64_to_f32);

def_rtl_fp_unary_prototype(fmv);
def_rtl_fp_unary_prototype(fneg);
def_rtl_fp_unary_prototype(fabs);
def_rtl(fclassd, rtlreg_t *dest, const fpreg_t *src1);

def_rtl(fpcall, uint32_t id, fpreg_t *dest, const fpreg_t *src1, const rtlreg_t *src2, uint32_t imm);
#endif

#endif
