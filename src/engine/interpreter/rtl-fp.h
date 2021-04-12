#ifndef __RTL_FP_H__
#define __RTL_FP_H__

#ifndef __RTL_RTL_H__
#error "Should be only included by <rtl/rtl.h>"
#endif

#include "c_op.h"
#include <memory/vaddr.h>
#include <softfloat.h>
#include <specialize.h>
#include <internals.h>
#include <fclass.h>


#define F32_SIGN ((uint32_t)1 << 31)
#define F64_SIGN ((uint64_t)1 << 63)

// ------------- helper functions -------------


// ------------- rtl code -------------

#ifdef __ISA_x86__
//set pfreg, it's a pointer to cpu.fpr in interpreter
static inline def_rtl(pfr,Operand* op,int i){
  uint8_t sti =  (cpu.ftop+i)&0x7;
  op->pfreg = &cpu.fpr[sti];
}

static inline void rtl_popftop(void){
  cpu.ftop++;
  cpu.ftop &= 0x7;
}
static inline void rtl_pop2ftop(void){
  cpu.ftop+=2;
  cpu.ftop &= 0x7;
}
static inline void rtl_pushftop(void){
  cpu.ftop--;
  cpu.ftop &= 0x7;
}

static inline def_rtl(lr_fsw, rtlreg_t* dest){
  *dest = cpu.fsw;
}
static inline def_rtl(sr_fsw, rtlreg_t* src){
  cpu.fsw = *src;
}
static inline def_rtl(lr_fcw, rtlreg_t* dest){
  *dest = cpu.fcw;
}
static inline def_rtl(sr_fcw, rtlreg_t* src){
  cpu.fcw = *src;
  rtlreg_t rm = BITS(cpu.fcw,11,10);
  switch (rm)
  {
  case 0:
    softfloat_roundingMode = softfloat_round_near_even;
    break;
  case 1:
    softfloat_roundingMode = softfloat_round_min;
    break;
  case 2:
    softfloat_roundingMode = softfloat_round_max;
    break;
  case 3:
    softfloat_roundingMode = softfloat_round_minMag;
    break;
  default:
    assert(0);
    break;
  }
}


static inline def_rtl(class387, rtlreg_t *dest, uint64_t *src){
  float64_t f;
  f.v = *src;
  uint32_t res = f64_classify(f);
  switch (res)
  {
  case 0x1://-inf
    *dest = 0x700;
    break;
  case 0x2://-normal
    *dest = 0x600;
    break;
  case 0x4://-subnormal
    *dest = 0x4600;
    break;
  case 0x8://-0
    *dest = 0x4200;
    break;
  case 0x10://+0
    *dest = 0x4000;
    break;
  case 0x20://+subnormal
    *dest = 0x4400;
    break;
  case 0x40://+normal
    *dest = 0x400;
    break;
  case 0x80://+inf
    *dest = 0x500;
    break;
  case 0x100://sNaN
  case 0x200://qNaN
    if(((bool) (*src>>63 ))){
      *dest = 0x300;
    }
    else{
      *dest = 0x100;
    }
    break;
  default:
    if(((bool) (*src>>63 ))){
      *dest = 0x200;
    }
    break;
  }
}

//load fpr from fpr pointer
static inline def_rtl(lfr, uint64_t* target_ptr, uint64_t* fptr){
  *target_ptr = *fptr;
}
//store fpr from fpr pointer
static inline def_rtl(sfr, uint64_t* fptr, uint64_t* fsrc){
  *fptr = *fsrc;
}

enum{fconst_1=0,fconst_l2t, fconst_l2e, fconst_pi,fconst_lg2, fconst_ln2, fconst_z};
static inline def_rtl(fld_const, uint64_t *fdest, int type){
  switch (type) {
  case fconst_1: *fdest = 0x3ff0000000000000ull; break;
  case fconst_z: *fdest = 0x0; break;
  case fconst_l2t: *fdest = 0x400A934F0979A372ull; break;
  case fconst_l2e: *fdest = 0x3FF71547652B82FEull; break;
  case fconst_pi:  *fdest = 0x400921FB54442D18ull; break;
  case fconst_lg2: *fdest = 0x3FD34413509F79FEull; break;
  case fconst_ln2: *fdest = 0x3FE62E42FEFA39EFull; break;
  default: assert(0);
  }
}

//load memory, convert to double-precision and store to fpr
static inline def_rtl(lmf, uint64_t *fdest, const rtlreg_t* addr, word_t offset) {
  switch (s->isa.fpu_MF)
  {
    case 0://32bit real
    {
      word_t val = vaddr_read(*addr + offset, 4);
      float32_t f32;
      float64_t f64;
      f32.v = val;
      f64 = f32_to_f64(f32);
      *fdest = f64.v;
      break;
    }
    case 1://32bit int
    {
      word_t val = vaddr_read(*addr + offset, 4);
      float64_t f64;
      f64 = i32_to_f64((int32_t)val);
      *fdest = f64.v;
      break;
  }
    case 2://64bit real
    {
      word_t val_lo = vaddr_read(*addr + offset, 4);
      word_t val_hi = vaddr_read(*addr + offset + 4, 4);
      uint64_t val = val_hi;
      val <<= 32;
      val |= val_lo;
      *fdest = val;
      break;
    }
    case 3://16bit int
    {
      word_t val = vaddr_read(*addr + offset, 2);
      float64_t f64;
      f64 = i32_to_f64((int16_t)val);
      *fdest = f64.v;
      break;
    }
    case 4://64bit int
    {
      word_t val_lo = vaddr_read(*addr + offset, 4);
      word_t val_hi = vaddr_read(*addr + offset + 4, 4);
      uint64_t val = val_hi;
      val <<= 32;
      val |= val_lo;
      float64_t f64;
      f64 = i64_to_f64((int64_t)val);
      *fdest = f64.v;
      break;
    }
    default:
      assert(0);
      break;
  }
}

//load memory, convert to double-precision and store to fpr
static inline def_rtl(smf, const rtlreg_t* addr, word_t offset, const uint64_t *fsrc) {
  switch (s->isa.fpu_MF)
  {
    case 0://32bit real
    {
      float32_t f32;
      float64_t f64;
      f64.v = *fsrc;
      f32 = f64_to_f32(f64);
      vaddr_write(*addr + offset, f32.v, 4);
      break;
    }
    case 1://32bit int
    {
      float64_t f64;
      f64.v = *fsrc;
      word_t val = (uint32_t)f64_to_i32(f64, softfloat_roundingMode, false);
      vaddr_write(*addr + offset, val, 4);
      break;
    }
    case 2://64bit real
    {
      word_t val = *fsrc & 0xFFFFFFFF;
      vaddr_write(*addr + offset, val, 4);
      val = *fsrc >> 32;
      vaddr_write(*addr + offset + 4, val, 4);
      break;
    }
    case 3://16bit int
    {
      float64_t f64;
      f64.v = *fsrc;
      word_t val = (uint32_t)f64_to_i32(f64, softfloat_roundingMode, false);
      assert(val <= 0xffff || val >= 0xffff0000);
      vaddr_write(*addr + offset, val, 2);
      break;
    }
    case 4://64bit int
    {
      word_t val;
      float64_t f64;
      f64.v = *fsrc;
      uint64_t ival = (uint64_t)f64_to_i64(f64, softfloat_roundingMode, false);
      val = ival & 0xFFFFFFFF;
      vaddr_write(*addr + offset, val, 4);
      val = ival >> 32;
      vaddr_write(*addr + offset + 4, val, 4);
      break;      
    }
    default:
      assert(0);
      break;
  }
}

static inline float64_t fprToF64(uint64_t r){
    float64_t f;
    f.v = r;
    return f;
}

#define BUILD_RTL_F(x) \
static inline def_rtl(concat(f64_, x), uint64_t* dest, uint64_t* src1, uint64_t* src2) { \
    *dest = concat(f64_, x)(fprToF64(*src1), fprToF64(*src2)).v; \
}

BUILD_RTL_F(add);
BUILD_RTL_F(sub);
BUILD_RTL_F(mul);
BUILD_RTL_F(div);

#define BUILD_RTL_FCMP(x) \
static inline def_rtl(concat(f64_, x), rtlreg_t* res, uint64_t* src1, uint64_t* src2) { \
  *res = concat(f64_, x)(fprToF64(*src1),fprToF64(*src2)); \
}

BUILD_RTL_FCMP(le);
BUILD_RTL_FCMP(eq);
BUILD_RTL_FCMP(lt);

#define F64_SIGN ((uint64_t)1 << 63)
static inline def_rtl(f64_chs, uint64_t* dest){
  *dest = *dest ^ F64_SIGN;
}
static inline def_rtl(f64_abs, uint64_t* dest){
  *dest = *dest & ~F64_SIGN;
}

#endif

#endif
