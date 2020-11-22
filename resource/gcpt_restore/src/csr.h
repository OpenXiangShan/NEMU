#ifndef __CSR_H__
#define __CSR_H__

//no mhartid here

 #define __INT_ONLY__

#define CSRS(f) \
  f(mstatus    , 0x300) f(misa       , 0x301) f(medeleg    , 0x302) f(mideleg    , 0x303) \
  f(mie        , 0x304) f(mtvec      , 0x305) f(mcounteren , 0x306) \
  f(mscratch   , 0x340) f(mepc       , 0x341) f(mcause     , 0x342) \
  f(mtval      , 0x343) f(mip        , 0x344) \
  f(pmpcfg0    , 0x3a0) \
  f(pmpaddr0   , 0x3b0) f(pmpaddr1   , 0x3b1) f(pmpaddr2   , 0x3b2) f(pmpaddr3   , 0x3b3) \
  f(sstatus    , 0x100) \
  f(sie        , 0x104) f(stvec      , 0x105) f(scounteren , 0x106) \
  f(sscratch   , 0x140) f(sepc       , 0x141) f(scause     , 0x142) \
  f(stval      , 0x143) f(sip        , 0x144) \
  f(satp       , 0x180)

#define FCSRS(f) \
  f(fflags     , 0x001) f(frm        , 0x002) f(fcsr       , 0x003)



#define CSRS_RESTORE(name, addr) \
  li t2, addr; \
  slli t2, t2, 3; \
  add t2, t0, t2; \
  ld t1, (t2); \
  csrw addr, t1; \

#endif
