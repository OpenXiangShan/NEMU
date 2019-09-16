#ifndef __CSR_H__
#define __CSR_H__

#include "common.h"

#define CSRS(f) \
  f(mstatus    , 0x300) f(mtvec      , 0x305) f(mepc       , 0x341) f(mcause     , 0x342)  \
  f(mie        , 0x304) f(satp       , 0x180)

#define CSR_STRUCT_START(name) \
  typedef union { \
    struct {

#define CSR_STRUCT_END(name) \
    }; \
    word_t val; \
  } concat(name, _t);

CSR_STRUCT_START(mstatus)
  uint64_t uie : 1;
  uint64_t sie : 1;
  uint64_t pad0: 1;
  uint64_t mie : 1;
  uint64_t upie: 1;
  uint64_t spie: 1;
  uint64_t pad1: 1;
  uint64_t mpie: 1;
  uint64_t spp : 1;
  uint64_t pad2: 2;
  uint64_t mpp : 2;
CSR_STRUCT_END(mstatus)

CSR_STRUCT_START(satp)
  uint64_t ppn :44;
  uint64_t asid:16;
  uint64_t mode: 4;
CSR_STRUCT_END(satp)

CSR_STRUCT_START(mtvec)
CSR_STRUCT_END(mtvec)

CSR_STRUCT_START(mcause)
CSR_STRUCT_END(mcause)

CSR_STRUCT_START(mepc)
CSR_STRUCT_END(mepc)

CSR_STRUCT_START(mie)
  uint64_t usie : 1;
  uint64_t ssie : 1;
  uint64_t hsie : 1;
  uint64_t msie : 1;
  uint64_t utie : 1;
  uint64_t stie : 1;
  uint64_t htie : 1;
  uint64_t mtie : 1;
  uint64_t ueie : 1;
  uint64_t seie : 1;
  uint64_t heie : 1;
  uint64_t meie : 1;
CSR_STRUCT_END(mie)

#define CSRS_DECL(name, val) extern concat(name, _t)* const name;
MAP(CSRS, CSRS_DECL)

word_t* csr_decode(uint32_t addr);

#endif
