/***************************************************************************************
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __CSR_H__
#define __CSR_H__

//no mhartid here

#define CSRS(f) \
  f(fcsr       , 0x003) \
  f(mstatus    , 0x300) f(misa       , 0x301) f(medeleg    , 0x302) f(mideleg    , 0x303) \
  f(mie        , 0x304) f(mtvec      , 0x305) f(mcounteren , 0x306) \
  f(mscratch   , 0x340) f(mepc       , 0x341) f(mcause     , 0x342) \
  f(mtval      , 0x343) f(mip        , 0x344) \
  f(pmpcfg0    , 0x3a0) f(pmpcfg2    , 0x3a2) \
  f(pmpaddr0   , 0x3b0) f(pmpaddr1   , 0x3b1) f(pmpaddr2   , 0x3b2) f(pmpaddr3   , 0x3b3) \
  f(pmpaddr4   , 0x3b4) f(pmpaddr5   , 0x3b5) f(pmpaddr6   , 0x3b6) f(pmpaddr7   , 0x3b7) \
  f(pmpaddr8   , 0x3b8) f(pmpaddr9   , 0x3b9) f(pmpaddr10  , 0x3ba) f(pmpaddr11  , 0x3bb) \
  f(pmpaddr12  , 0x3bc) f(pmpaddr13  , 0x3bd) f(pmpaddr14  , 0x3be) f(pmpaddr15  , 0x3bf) \
  f(stvec      , 0x105) f(scounteren , 0x106) \
  f(sscratch   , 0x140) f(sepc       , 0x141) f(scause     , 0x142) \
  f(stval      , 0x143) \
  f(satp       , 0x180)




#define CSRS_RESTORE(name, addr) \
  li t2, addr; \
  slli t2, t2, 3; \
  add t2, t0, t2; \
  ld t1, (t2); \
  csrw addr, t1; \

#endif
