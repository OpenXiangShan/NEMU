/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include <common.h>

#ifdef CONFIG_RV_SVINVAL
#define CUSTOM_CSR(f) \
  f(srnctl     , 0x5c4)
#else
#define CUSTOM_CSR(f)
#endif

// SHARE mode does not support mtime
#ifdef CONFIG_RV_PMP_CSR
#define CSRS_PMP(f) \
  f(pmpcfg0    , 0x3a0) f(pmpcfg2    , 0x3a2) \
  f(pmpaddr0   , 0x3b0) f(pmpaddr1   , 0x3b1) f(pmpaddr2   , 0x3b2) f(pmpaddr3   , 0x3b3) \
  f(pmpaddr4   , 0x3b4) f(pmpaddr5   , 0x3b5) f(pmpaddr6   , 0x3b6) f(pmpaddr7   , 0x3b7) \
  f(pmpaddr8   , 0x3b8) f(pmpaddr9   , 0x3b9) f(pmpaddr10  , 0x3ba) f(pmpaddr11  , 0x3bb) \
  f(pmpaddr12  , 0x3bc) f(pmpaddr13  , 0x3bd) f(pmpaddr14  , 0x3be) f(pmpaddr15  , 0x3bf)
#else
#define CSRS_PMP(f)
#endif // CONFIG_RV_PMP_CSR

#ifdef CONFIG_RV_SPMP_CSR
#define CSRS_SPMP(f) \
  f(spmpcfg0    , 0x1a0) f(spmpcfg2, 0x1a2)\
  f(spmpaddr0   , 0x1b0) f(spmpaddr1   , 0x1b1) f(spmpaddr2   , 0x1b2) f(spmpaddr3   , 0x1b3) \
  f(spmpaddr4   , 0x1b4) f(spmpaddr5   , 0x1b5) f(spmpaddr6   , 0x1b6) f(spmpaddr7   , 0x1b7) \
  f(spmpaddr8   , 0x1b8) f(spmpaddr9   , 0x1b9) f(spmpaddr10  , 0x1ba) f(spmpaddr11  , 0x1bb) \
  f(spmpaddr12  , 0x1bc) f(spmpaddr13  , 0x1bd) f(spmpaddr14  , 0x1be) f(spmpaddr15  , 0x1bf)
#else
#define CSRS_SPMP(f)
#endif // CONFIG_RV_SPMP_CSR

#ifndef CONFIG_SHARE
#define CSRS(f) \
  f(mstatus    , 0x300) f(misa       , 0x301) f(medeleg    , 0x302) f(mideleg    , 0x303) \
  f(mie        , 0x304) f(mtvec      , 0x305) f(mcounteren , 0x306) \
  f(mscratch   , 0x340) f(mepc       , 0x341) f(mcause     , 0x342) \
  f(mtval      , 0x343) f(mip        , 0x344) \
  CSRS_PMP(f) \
  CSRS_SPMP(f) \
  f(mhartid    , 0xf14) \
  f(sstatus    , 0x100) \
  f(sie        , 0x104) f(stvec      , 0x105) f(scounteren , 0x106) \
  f(sscratch   , 0x140) f(sepc       , 0x141) f(scause     , 0x142) \
  f(stval      , 0x143) f(sip        , 0x144) \
  f(satp       , 0x180) \
  CUSTOM_CSR(f) \
  f(fflags     , 0x001) f(frm        , 0x002) f(fcsr       , 0x003) \
  f(mtime      , 0xc01)
#else
#define CSRS(f) \
  f(mstatus    , 0x300) f(misa       , 0x301) f(medeleg    , 0x302) f(mideleg    , 0x303) \
  f(mie        , 0x304) f(mtvec      , 0x305) f(mcounteren , 0x306) \
  f(mscratch   , 0x340) f(mepc       , 0x341) f(mcause     , 0x342) \
  f(mtval      , 0x343) f(mip        , 0x344) \
  CSRS_PMP(f) \
  f(mhartid    , 0xf14) \
  f(sstatus    , 0x100) \
  f(sie        , 0x104) f(stvec      , 0x105) f(scounteren , 0x106) \
  f(sscratch   , 0x140) f(sepc       , 0x141) f(scause     , 0x142) \
  f(stval      , 0x143) f(sip        , 0x144) \
  f(satp       , 0x180) \
  CUSTOM_CSR(f) \
  f(fflags     , 0x001) f(frm        , 0x002) f(fcsr       , 0x003)
#endif

#ifdef CONFIG_RVV
  #define VCSRS(f) \
  f(vstart, 0x008) \
  f(vxsat, 0x009) \
  f(vxrm, 0x00a) \
  f(vcsr, 0x00f) \
  f(vl, 0xc20) \
  f(vtype, 0xc21) \
  f(vlenb, 0xc22)
#endif

#ifdef CONFIG_RV_ARCH_CSRS
  #define ARCH_CSRS(f) \
  f(mvendorid  , 0xf11) f(marchid    , 0xf12) f(mimpid     , 0xf13)
#endif

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
  uint64_t vs: 2;
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

/** pmp */

#define PMP_R     0x01
#define PMP_W     0x02
#define PMP_X     0x04
#define PMP_A     0x18
#define PMP_L     0x80
#define PMP_SHIFT 2
#define PMP_PLATFORMGARIN 12 // log2(4KB)

#define PMP_TOR   0x08
#define PMP_NA4   0x10
#define PMP_NAPOT 0x18

#define CSR_PMPCFG0 0x3a0
#define CSR_PMPCFG2 0x3a2
#define CSR_PMPADDR0 0x3b0
#define CSR_PMPADDR1 0x3b1
#define CSR_PMPADDR2 0x3b2
#define CSR_PMPADDR3 0x3b3
#define CSR_PMPADDR4 0x3b4
#define CSR_PMPADDR5 0x3b5
#define CSR_PMPADDR6 0x3b6
#define CSR_PMPADDR7 0x3b7
#define CSR_PMPADDR8 0x3b8
#define CSR_PMPADDR9 0x3b9
#define CSR_PMPADDR10 0x3ba
#define CSR_PMPADDR11 0x3bb
#define CSR_PMPADDR12 0x3bc
#define CSR_PMPADDR13 0x3bd
#define CSR_PMPADDR14 0x3be
#define CSR_PMPADDR15 0x3bf

/** spmp */

//R/W/X/A/L field in sPMP configuration registers
#define SPMP_R     0x01
#define SPMP_W     0x02
#define SPMP_X     0x04
#define SPMP_A     0x18
#define SPMP_S     0x80       
#define SPMP_SHIFT 2
#define SPMP_PLATFORMGARIN 12 // log2(4KB)

//encoding of A field in sPMP configuration registers
#define SPMP_TOR   0x08
#define SPMP_NA4   0x10
#define SPMP_NAPOT 0x18

#define SPMP_OFF   0x00
#define SPMP_NO_PERM  0

#define CSR_SPMPCFG0        0x1a0
#define CSR_SPMPCFG2        0x1a2
#define CSR_SPMPADDR0       0x1b0
#define CSR_SPMPADDR1       0x1b1
#define CSR_SPMPADDR2       0x1b2
#define CSR_SPMPADDR3       0x1b3
#define CSR_SPMPADDR4       0x1b4
#define CSR_SPMPADDR5       0x1b5
#define CSR_SPMPADDR6       0x1b6
#define CSR_SPMPADDR7       0x1b7
#define CSR_SPMPADDR8       0x1b8
#define CSR_SPMPADDR9       0x1b9
#define CSR_SPMPADDR10      0x1ba
#define CSR_SPMPADDR11      0x1bb
#define CSR_SPMPADDR12      0x1bc
#define CSR_SPMPADDR13      0x1bd
#define CSR_SPMPADDR14      0x1be
#define CSR_SPMPADDR15      0x1bf

// This is the maximum PMP register allowed.
// If you need to change the number of actual PMP registers,
// please set CONFIG_RV_PMP_NUM in the config file.
#define MAX_NUM_PMP 16

CSR_STRUCT_START(pmpcfg0)
CSR_STRUCT_END(pmpcfg0)


CSR_STRUCT_START(pmpcfg2)
CSR_STRUCT_END(pmpcfg2)

CSR_STRUCT_START(pmpaddr0)
CSR_STRUCT_END(pmpaddr0)

CSR_STRUCT_START(pmpaddr1)
CSR_STRUCT_END(pmpaddr1)

CSR_STRUCT_START(pmpaddr2)
CSR_STRUCT_END(pmpaddr2)

CSR_STRUCT_START(pmpaddr3)
CSR_STRUCT_END(pmpaddr3)

CSR_STRUCT_START(pmpaddr4)
CSR_STRUCT_END(pmpaddr4)

CSR_STRUCT_START(pmpaddr5)
CSR_STRUCT_END(pmpaddr5)

CSR_STRUCT_START(pmpaddr6)
CSR_STRUCT_END(pmpaddr6)

CSR_STRUCT_START(pmpaddr7)
CSR_STRUCT_END(pmpaddr7)

CSR_STRUCT_START(pmpaddr8)
CSR_STRUCT_END(pmpaddr8)

CSR_STRUCT_START(pmpaddr9)
CSR_STRUCT_END(pmpaddr9)

CSR_STRUCT_START(pmpaddr10)
CSR_STRUCT_END(pmpaddr10)

CSR_STRUCT_START(pmpaddr11)
CSR_STRUCT_END(pmpaddr11)

CSR_STRUCT_START(pmpaddr12)
CSR_STRUCT_END(pmpaddr12)

CSR_STRUCT_START(pmpaddr13)
CSR_STRUCT_END(pmpaddr13)

CSR_STRUCT_START(pmpaddr14)
CSR_STRUCT_END(pmpaddr14)

CSR_STRUCT_START(pmpaddr15)
CSR_STRUCT_END(pmpaddr15)

// This is the maximum SPMP register allowed.
// If you need to change the number of actual SPMP registers,
// please set CONFIG_RV_SPMP_NUM in the config file.
#define MAX_NUM_SPMP 16

CSR_STRUCT_START(spmpcfg0)
CSR_STRUCT_END(spmpcfg0)


CSR_STRUCT_START(spmpcfg2)
CSR_STRUCT_END(spmpcfg2)

CSR_STRUCT_START(spmpaddr0)
CSR_STRUCT_END(spmpaddr0)

CSR_STRUCT_START(spmpaddr1)
CSR_STRUCT_END(spmpaddr1)

CSR_STRUCT_START(spmpaddr2)
CSR_STRUCT_END(spmpaddr2)

CSR_STRUCT_START(spmpaddr3)
CSR_STRUCT_END(spmpaddr3)

CSR_STRUCT_START(spmpaddr4)
CSR_STRUCT_END(spmpaddr4)

CSR_STRUCT_START(spmpaddr5)
CSR_STRUCT_END(spmpaddr5)

CSR_STRUCT_START(spmpaddr6)
CSR_STRUCT_END(spmpaddr6)

CSR_STRUCT_START(spmpaddr7)
CSR_STRUCT_END(spmpaddr7)

CSR_STRUCT_START(spmpaddr8)
CSR_STRUCT_END(spmpaddr8)

CSR_STRUCT_START(spmpaddr9)
CSR_STRUCT_END(spmpaddr9)

CSR_STRUCT_START(spmpaddr10)
CSR_STRUCT_END(spmpaddr10)

CSR_STRUCT_START(spmpaddr11)
CSR_STRUCT_END(spmpaddr11)

CSR_STRUCT_START(spmpaddr12)
CSR_STRUCT_END(spmpaddr12)

CSR_STRUCT_START(spmpaddr13)
CSR_STRUCT_END(spmpaddr13)

CSR_STRUCT_START(spmpaddr14)
CSR_STRUCT_END(spmpaddr14)

CSR_STRUCT_START(spmpaddr15)
CSR_STRUCT_END(spmpaddr15)

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

#define SATP_ASID_LEN 16 // max is 16
#define SATP_PADDR_LEN (CONFIG_PADDRBITS-12) // max is 44
#define SATP_ASID_MAX_LEN 16
#define SATP_PADDR_MAX_LEN 44

#define SATP_MODE_MASK (8UL << (SATP_ASID_MAX_LEN + SATP_PADDR_MAX_LEN))
#define SATP_ASID_MASK (((1L << SATP_ASID_LEN)-1) << SATP_PADDR_MAX_LEN)
#define SATP_PADDR_MASK ((1L << SATP_PADDR_LEN)-1)

#define SATP_MASK (SATP_MODE_MASK | SATP_ASID_MASK | SATP_PADDR_MASK)
#define MASKED_SATP(x) (SATP_MASK & x)

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

#ifdef CONFIG_RV_SVINVAL
// NOTE: srcctl is a supervisor custom read/write csr
// to fix xiangshan that:
// rnctl: move elimination,
// svinval: one vm extension
CSR_STRUCT_START(srnctl)
  uint64_t rnctrl  : 1;
  uint64_t svinval : 1;
  uint64_t reserve :63;
CSR_STRUCT_END(srnctl)
#endif

CSR_STRUCT_START(fflags)
CSR_STRUCT_END(fflags)

CSR_STRUCT_START(frm)
CSR_STRUCT_END(frm)

CSR_STRUCT_START(fcsr)
  union {
    struct {
      uint64_t nv: 1;
      uint64_t dz: 1;
      uint64_t of: 1;
      uint64_t uf: 1;
      uint64_t nx: 1;
      uint64_t frm : 3;
    };
    struct {
      uint64_t val: 5;
    } fflags;
  };
CSR_STRUCT_END(fcsr)

CSR_STRUCT_START(mtime)
CSR_STRUCT_END(mtime)

#ifdef CONFIG_RVV
// TODO: implement these vcsr
#define IDXVSTART 0x008
#define IDXVXSAT  0x009
#define IDXVXRM   0x00a
#define IDXVL     0xc20
#define IDXVTYPE  0xc21
#define IDXVLENB  0xc22

CSR_STRUCT_START(vstart)
CSR_STRUCT_END(vstart)

CSR_STRUCT_START(vxsat)
  uint64_t sat :  1;
  uint64_t pad : 63;
CSR_STRUCT_END(vxsat)

CSR_STRUCT_START(vxrm)
  uint64_t rm  :  2;
  uint64_t pad : 62;
CSR_STRUCT_END(vxrm)

CSR_STRUCT_START(vcsr)
  uint64_t vxsat :  1;
  uint64_t vxrm  :  2;
  uint64_t pad   : 61;
CSR_STRUCT_END(vcsr)

CSR_STRUCT_START(vl)
CSR_STRUCT_END(vl)

CSR_STRUCT_START(vtype)
  uint64_t vlmul :  3;
  uint64_t vsew  :  3;
  uint64_t vediv :  2;
  uint64_t pad   : 55;
  uint64_t vill  :  1;
CSR_STRUCT_END(vtype)

CSR_STRUCT_START(vlenb)
CSR_STRUCT_END(vlenb)

rtlreg_t check_vsetvl(rtlreg_t vtype_req, rtlreg_t vl_req, int mode);
rtlreg_t get_mask(int reg, int idx, uint64_t vsew, uint64_t vlmul);
void set_mask(uint32_t reg, int idx, uint64_t mask, uint64_t vsew, uint64_t vlmul);

#endif // CONFIG_RVV

#ifdef CONFIG_RV_ARCH_CSRS
CSR_STRUCT_START(mvendorid)
CSR_STRUCT_END(mvendorid)

CSR_STRUCT_START(marchid)
CSR_STRUCT_END(marchid)

CSR_STRUCT_START(marchid)
CSR_STRUCT_END(mimpid)
#endif // CONFIG_RV_ARCH_CSRS

#define CSRS_DECL(name, addr) extern concat(name, _t)* const name;
MAP(CSRS, CSRS_DECL)
#ifdef CONFIG_RVV
  MAP(VCSRS, CSRS_DECL)
#endif // CONFIG_RVV
#ifdef CONFIG_RV_ARCH_CSRS
  MAP(ARCH_CSRS, CSRS_DECL)
#endif // CONFIG_RV_ARCH_CSRS

word_t csrid_read(uint32_t csrid);

// PMP
uint8_t pmpcfg_from_index(int idx);
word_t pmpaddr_from_index(int idx);
word_t pmpaddr_from_csrid(int id);
word_t pmp_tor_mask();

// sPMP
uint8_t spmpcfg_from_index(int idx);
word_t spmpaddr_from_index(int idx);
word_t spmpaddr_from_csrid(int id);
word_t spmp_tor_mask();

#endif
