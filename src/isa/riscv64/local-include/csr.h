#ifndef __CSR_H__
#define __CSR_H__

#include <common.h>

#define CSRS(f) \
  f(mstatus    , 0x300) f(misa       , 0x301) f(medeleg    , 0x302) f(mideleg    , 0x303) \
  f(mie        , 0x304) f(mtvec      , 0x305) f(mcounteren , 0x306) \
  f(mscratch   , 0x340) f(mepc       , 0x341) f(mcause     , 0x342) \
  f(mtval      , 0x343) f(mip        , 0x344) \
  f(pmpcfg0    , 0x3a0) \
  f(pmpaddr0   , 0x3b0) f(pmpaddr1   , 0x3b1) f(pmpaddr2   , 0x3b2) f(pmpaddr3   , 0x3b3) \
  f(mhartid    , 0xf14) \
  f(sstatus    , 0x100) \
  f(sie        , 0x104) f(stvec      , 0x105) f(scounteren , 0x106) \
  f(sscratch   , 0x140) f(sepc       , 0x141) f(scause     , 0x142) \
  f(stval      , 0x143) f(sip        , 0x144) \
  f(satp       , 0x180) \
  f(fflags     , 0x001) f(frm        , 0x002) f(fcsr       , 0x003)

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
  uint64_t fs  : 2;
  uint64_t xs  : 2;
  uint64_t mprv: 1;
  uint64_t sum : 1;
  uint64_t mxr : 1;
  uint64_t tvm : 1;
  uint64_t tw  : 1;
  uint64_t tsr : 1;
  uint64_t pad3: 9;
  uint64_t uxl : 2;
  uint64_t sxl : 2;
  uint64_t sbe : 1;
  uint64_t mbe : 1;
  uint64_t pad4:25;
  uint64_t sd  : 1;
CSR_STRUCT_END(mstatus)

CSR_STRUCT_START(misa)
  uint64_t extensions: 26;
  uint64_t pad       : 36;
  uint64_t mxl       :  2;
CSR_STRUCT_END(misa)

CSR_STRUCT_START(mtvec)
CSR_STRUCT_END(mtvec)

CSR_STRUCT_START(mcounteren)
CSR_STRUCT_END(mcounteren)

CSR_STRUCT_START(mcause)
  uint64_t code:63;
  uint64_t intr: 1;
CSR_STRUCT_END(mcause)

CSR_STRUCT_START(mepc)
CSR_STRUCT_END(mepc)

CSR_STRUCT_START(medeleg)
CSR_STRUCT_END(medeleg)

CSR_STRUCT_START(mideleg)
CSR_STRUCT_END(mideleg)

CSR_STRUCT_START(mhartid)
CSR_STRUCT_END(mhartid)

CSR_STRUCT_START(mscratch)
CSR_STRUCT_END(mscratch)

CSR_STRUCT_START(mtval)
CSR_STRUCT_END(mtval)

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

CSR_STRUCT_START(mip)
  uint64_t usip : 1;
  uint64_t ssip : 1;
  uint64_t hsip : 1;
  uint64_t msip : 1;
  uint64_t utip : 1;
  uint64_t stip : 1;
  uint64_t htip : 1;
  uint64_t mtip : 1;
  uint64_t ueip : 1;
  uint64_t seip : 1;
  uint64_t heip : 1;
  uint64_t meip : 1;
CSR_STRUCT_END(mip)

CSR_STRUCT_START(pmpcfg0)
CSR_STRUCT_END(pmpcfg0)

CSR_STRUCT_START(pmpaddr0)
CSR_STRUCT_END(pmpaddr0)

CSR_STRUCT_START(pmpaddr1)
CSR_STRUCT_END(pmpaddr1)

CSR_STRUCT_START(pmpaddr2)
CSR_STRUCT_END(pmpaddr2)

CSR_STRUCT_START(pmpaddr3)
CSR_STRUCT_END(pmpaddr3)

CSR_STRUCT_START(sstatus)
  uint64_t uie : 1;
  uint64_t sie : 1;
  uint64_t pad0: 2;
  uint64_t upie: 1;
  uint64_t spie: 1;
  uint64_t pad1: 2;
  uint64_t spp : 1;
  uint64_t pad2: 4;
CSR_STRUCT_END(sstatus)

CSR_STRUCT_START(stvec)
CSR_STRUCT_END(stvec)

CSR_STRUCT_START(scounteren)
CSR_STRUCT_END(scounteren)

CSR_STRUCT_START(sie)
  uint64_t usie : 1;
  uint64_t ssie : 1;
  uint64_t pad0 : 2;
  uint64_t utie : 1;
  uint64_t stie : 1;
  uint64_t pad1 : 2;
  uint64_t ueie : 1;
  uint64_t seie : 1;
  uint64_t pad2 : 2;
CSR_STRUCT_END(sie)

CSR_STRUCT_START(sip)
  uint64_t usip : 1;
  uint64_t ssip : 1;
  uint64_t pad0 : 2;
  uint64_t utip : 1;
  uint64_t stip : 1;
  uint64_t pad1 : 2;
  uint64_t ueip : 1;
  uint64_t seip : 1;
  uint64_t pad2 : 2;
CSR_STRUCT_END(sip)


CSR_STRUCT_START(satp)
  uint64_t ppn :44;
  uint64_t asid:16;
  uint64_t mode: 4;
CSR_STRUCT_END(satp)

CSR_STRUCT_START(scause)
  uint64_t code:63;
  uint64_t intr: 1;
CSR_STRUCT_END(scause)

CSR_STRUCT_START(sepc)
CSR_STRUCT_END(sepc)

CSR_STRUCT_START(stval)
CSR_STRUCT_END(stval)

CSR_STRUCT_START(sscratch)
CSR_STRUCT_END(sscratch)

CSR_STRUCT_START(fflags)
  uint64_t nv: 1;
  uint64_t dz: 1;
  uint64_t of: 1;
  uint64_t uf: 1;
  uint64_t nx: 1;
CSR_STRUCT_END(fflags)

CSR_STRUCT_START(frm)
CSR_STRUCT_END(frm)

CSR_STRUCT_START(fcsr)
CSR_STRUCT_END(fcsr)

#define CSRS_DECL(name, addr) extern concat(name, _t)* const name;
MAP(CSRS, CSRS_DECL)

void csr_read(rtlreg_t *dest, uint32_t addr);
void csr_write(uint32_t addr, rtlreg_t *src);

enum { MODE_U = 0, MODE_S, MODE_H, MODE_M };
void change_mode(uint8_t m);
bool ptw_is_safe(vaddr_t vaddr);

#endif
