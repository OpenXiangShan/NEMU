#ifndef __RTL_FP_H__
#define __RTL_FP_H__

#ifndef __RTL_RTL_H__
#error "Should be only included by <rtl/rtl.h>"
#endif

#include "c_op.h"
#include <memory/vaddr.h>
#include "softfloat/softfloat.h"
#include "softfloat/specialize.h"
#include "softfloat/internals.h"


#define F32_SIGN ((uint32_t)1 << 31)
#define F64_SIGN ((uint64_t)1 << 63)

// ------------- helper functions -------------


// ------------- rtl code -------------

//set pfreg, it's a pointer to cpu.fpr in interpreter
//it will be a pointer to addr GPR in sdi
static inline make_rtl(pfr,Operand* op,int i){
  uint8_t sti =  (cpu.ftop+i)&0x7;
  op->pfreg = &cpu.fpr[sti];
}
//load fpr from fpr pointer
static inline make_rtl(lfr, uint64_t* target_ptr, uint64_t* fptr){
  *target_ptr = *fptr;
}
//store fpr from fpr pointer
static inline make_rtl(sfr, uint64_t* fptr, uint64_t* fsrc){
  *fptr = *fsrc;
}

//load memory, convert to double-precision and store to fpr
static inline make_rtl(lmf, uint64_t *fdest, const rtlreg_t* addr, word_t offset) {
  switch (s->isa.fpu_MF)
  {
    case 0://32bit real
      word_t val = vaddr_read(*addr + offset, 4);
      float32_t f32;
      float64_t f64;
      f32.v = val;
      f64 = f32_to_f64(f32);
      *fdest = f64.v;
      break;
    case 1://32bit int
      word_t val = vaddr_read(*addr + offset, 4);
      float64_t f64;
      f64 = i32_to_f64((int32_t)val);
      *fdest = f64.v;
      break;
    case 2://64bit real
      word_t val_lo = vaddr_read(*addr + offset, 4);
      word_t val_hi = vaddr_read(*addr + offset + 4, 4);
      uint64_t val = val_hi;
      val <<= 32;
      val |= val_lo;
      *fdest = val;
      break;
    case 3://16bit int
      word_t val = vaddr_read(*addr + offset, 2);
      float64_t f64;
      f64 = i32_to_f64((int16_t)val);
      *fdest = f64.v;
      break;
    case 4://64bit int
      word_t val_lo = vaddr_read(*addr + offset, 4);
      word_t val_hi = vaddr_read(*addr + offset + 4, 4);
      uint64_t val = val_hi;
      val <<= 32;
      val |= val_lo;
      float64_t f64;
      f64 = i64_to_f64((int64_t)val);
      *fdest = f64.v;
      break;      
    default:
      assert(0);
      break;
  }
}

//load memory, convert to double-precision and store to fpr
static inline make_rtl(smf, const rtlreg_t* addr, word_t offset, const uint64_t *fsrc) {
  switch (s->isa.fpu_MF)
  {
    case 0://32bit real
      float32_t f32;
      float64_t f64;
      f64.v = *fsrc;
      f32 = f64_to_f32(f64);
      vaddr_write(*addr + offset, f32.v, 4);
      break;
    case 1://32bit int
      word_t val;
      float64_t f64;
      f64.v = *fsrc;
      word_t val = (uint32_t)f64_to_i32(f64, softfloat_round_near_even, false);
      vaddr_write(*addr + offset, val, 4);
      break;
    case 2://64bit real
      word_t val = *fsrc & 0xFFFFFFFF;
      vaddr_write(*addr + offset, val, 4);
      val = *fsrc >> 32;
      vaddr_write(*addr + offset + 4, val, 4);
      break;
    case 3://16bit int
      float64_t f64;
      f64.v = *fsrc;
      word_t val = (uint32_t)f64_to_i32(f64, softfloat_round_near_even, false);
      assert(val <= 0xffff || val >= 0xffff0000);
      vaddr_write(*addr + offset, val, 2);
      break;
    case 4://64bit int
      word_t val;
      float64_t f64;
      f64.v = *fsrc;
      uint64_t ival = (uint64_t)f64_to_i64(f64, softfloat_round_near_even, false);
      word_t val = ival & 0xFFFFFFFF;
      vaddr_write(*addr + offset, val, 4);
      val = ival >> 32;
      vaddr_write(*addr + offset + 4, val, 4);
      break;      
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
static inline make_rtl(concat(f, x), uint64_t* dest, uint64_t* src1, uint64_t* src2) { \
    *dest = concat(f64_, x)(fprToF64(*src1), fprToF64(*src2)).v; \
}

BUILD_EXEC_F(add);
BUILD_EXEC_F(sub);
BUILD_EXEC_F(mul);
BUILD_EXEC_F(div);

#endif