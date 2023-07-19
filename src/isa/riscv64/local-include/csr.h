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

// CSRs for DASICS protection mechanism
#ifdef CONFIG_RV_DASICS
#define DASICS_CSRS(f) \
  f(dsmcfg,      0xbc0) f(dsmbound0,   0xbc2) f(dsmbound1,   0xbc3) \
  f(dumcfg,      0x9e0) f(dumbound0,   0x9e2) f(dumbound1,   0x9e3) \
  f(dlcfg0,      0x880) \
  f(dlbound0,    0x890) f(dlbound1,    0x891) f(dlbound2,    0x892) f(dlbound3,    0x893) \
  f(dlbound4,    0x894) f(dlbound5,    0x895) f(dlbound6,    0x896) f(dlbound7,    0x897) \
  f(dlbound8,    0x898) f(dlbound9,    0x899) f(dlbound10,   0x89a) f(dlbound11,   0x89b) \
  f(dlbound12,   0x89c) f(dlbound13,   0x89d) f(dlbound14,   0x89e) f(dlbound15,   0x89f) \
  f(dlbound16,   0x8a0) f(dlbound17,   0x8a1) f(dlbound18,   0x8a2) f(dlbound19,   0x8a3) \
  f(dlbound20,   0x8a4) f(dlbound21,   0x8a5) f(dlbound22,   0x8a6) f(dlbound23,   0x8a7) \
  f(dlbound24,   0x8a8) f(dlbound25,   0x8a9) f(dlbound26,   0x8aa) f(dlbound27,   0x8ab) \
  f(dlbound28,   0x8ac) f(dlbound29,   0x8ad) f(dlbound30,   0x8ae) f(dlbound31,   0x8af) \
  f(dmaincall,   0x8b0) f(dretpc,      0x8b1) f(dretpcfz,    0x8b2) \
  f(djbound0lo,  0x8c0) f(djbound0hi,  0x8c1) f(djbound1lo,  0x8c2) f(djbound1hi,  0x8c3) \
  f(djbound2lo,  0x8c4) f(djbound2hi,  0x8c5) f(djbound3lo,  0x8c6) f(djbound3hi,  0x8c7) \
  f(djcfg,       0x8c8)
#else  // CONFIG_RV_DASICS
#define DASICS_CSRS(f)
#endif  // !CONFIG_RV_DASICS

#ifndef CONFIG_SHARE
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

#ifdef CONFIG_RVN
#define NCSRS(f) \
  f(ustatus,     0x000) f(uie,         0x004) f(utvec,       0x005) \
  f(uscratch,    0x040) f(uepc,        0x041) f(ucause,      0x042) \
  f(utval,       0x043) f(uip,         0x044) \
  f(sedeleg,     0x102) f(sideleg,     0x103)
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

#ifdef CONFIG_RVN
CSR_STRUCT_START(ustatus)
  uint64_t uie :1;
  uint64_t pad :3;
  uint64_t upie:1;
CSR_STRUCT_END(ustatus)

CSR_STRUCT_START(uie)
  uint64_t usie:1;
  uint64_t pad0:3;
  uint64_t utie:1;
  uint64_t pad1:3;
  uint64_t ueie:1;
  uint64_t pad2:3;
CSR_STRUCT_END(uie)

CSR_STRUCT_START(utvec)
CSR_STRUCT_END(utvec)

CSR_STRUCT_START(uscratch)
CSR_STRUCT_END(uscratch)

CSR_STRUCT_START(uepc)
CSR_STRUCT_END(uepc)

CSR_STRUCT_START(ucause)
CSR_STRUCT_END(ucause)

CSR_STRUCT_START(utval)
CSR_STRUCT_END(utval)

CSR_STRUCT_START(uip)
  uint64_t usip:1;
  uint64_t pad0:3;
  uint64_t utip:1;
  uint64_t pad1:3;
  uint64_t ueip:1;
  uint64_t pad2:3;
CSR_STRUCT_END(uip)

CSR_STRUCT_START(sedeleg)
CSR_STRUCT_END(sedeleg)

CSR_STRUCT_START(sideleg)
CSR_STRUCT_END(sideleg)

#endif  // CONFIG_RVN

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

#ifdef CONFIG_RV_DASICS

#define MCFG_UCLS   0x8ul
#define MCFG_SCLS   0x4ul
#define MCFG_UENA   0X2ul
#define MCFG_SENA   0x1ul

CSR_STRUCT_START(dsmcfg)
  uint64_t mcfg_sena:1;
  uint64_t mcfg_uena:1;
  uint64_t mcfg_scls:1;
  uint64_t mcfg_ucls:1;
CSR_STRUCT_END(dsmcfg)

CSR_STRUCT_START(dsmbound0)
CSR_STRUCT_END(dsmbound0)

CSR_STRUCT_START(dsmbound1)
CSR_STRUCT_END(dsmbound1)

CSR_STRUCT_START(dumcfg)
  uint64_t pad0     :1;
  uint64_t mcfg_uena:1;
  uint64_t pad1     :1;
  uint64_t mcfg_ucls:1;
CSR_STRUCT_END(dumcfg)

CSR_STRUCT_START(dumbound0)
CSR_STRUCT_END(dumbound0)

CSR_STRUCT_START(dumbound1)
CSR_STRUCT_END(dumbound1)

#define CSR_DLCFG0   0x880
#define CSR_DLBOUND0 0x890
#define CSR_DLBOUND1 0x891
#define CSR_DJBOUND0 0x8c0
#define CSR_DJCFG    0x8c8

#define LIBCFG_MASK 0xful
#define LIBCFG_V    0x8ul
#define LIBCFG_R    0x2ul
#define LIBCFG_W    0x1ul

#define JUMPCFG_MASK 0xfffful
#define JUMPCFG_V 0x1ul

#define MAX_DASICS_LIBBOUNDS 16
#define MAX_DASICS_JUMPBOUNDS 4

CSR_STRUCT_START(dlcfg0)
CSR_STRUCT_END(dlcfg0)

CSR_STRUCT_START(dlbound0)
CSR_STRUCT_END(dlbound0)

CSR_STRUCT_START(dlbound1)
CSR_STRUCT_END(dlbound1)

CSR_STRUCT_START(dlbound2)
CSR_STRUCT_END(dlbound2)

CSR_STRUCT_START(dlbound3)
CSR_STRUCT_END(dlbound3)

CSR_STRUCT_START(dlbound4)
CSR_STRUCT_END(dlbound4)

CSR_STRUCT_START(dlbound5)
CSR_STRUCT_END(dlbound5)

CSR_STRUCT_START(dlbound6)
CSR_STRUCT_END(dlbound6)

CSR_STRUCT_START(dlbound7)
CSR_STRUCT_END(dlbound7)

CSR_STRUCT_START(dlbound8)
CSR_STRUCT_END(dlbound8)

CSR_STRUCT_START(dlbound9)
CSR_STRUCT_END(dlbound9)

CSR_STRUCT_START(dlbound10)
CSR_STRUCT_END(dlbound10)

CSR_STRUCT_START(dlbound11)
CSR_STRUCT_END(dlbound11)

CSR_STRUCT_START(dlbound12)
CSR_STRUCT_END(dlbound12)

CSR_STRUCT_START(dlbound13)
CSR_STRUCT_END(dlbound13)

CSR_STRUCT_START(dlbound14)
CSR_STRUCT_END(dlbound14)

CSR_STRUCT_START(dlbound15)
CSR_STRUCT_END(dlbound15)

CSR_STRUCT_START(dlbound16)
CSR_STRUCT_END(dlbound16)

CSR_STRUCT_START(dlbound17)
CSR_STRUCT_END(dlbound17)

CSR_STRUCT_START(dlbound18)
CSR_STRUCT_END(dlbound18)

CSR_STRUCT_START(dlbound19)
CSR_STRUCT_END(dlbound19)

CSR_STRUCT_START(dlbound20)
CSR_STRUCT_END(dlbound20)

CSR_STRUCT_START(dlbound21)
CSR_STRUCT_END(dlbound21)

CSR_STRUCT_START(dlbound22)
CSR_STRUCT_END(dlbound22)

CSR_STRUCT_START(dlbound23)
CSR_STRUCT_END(dlbound23)

CSR_STRUCT_START(dlbound24)
CSR_STRUCT_END(dlbound24)

CSR_STRUCT_START(dlbound25)
CSR_STRUCT_END(dlbound25)

CSR_STRUCT_START(dlbound26)
CSR_STRUCT_END(dlbound26)

CSR_STRUCT_START(dlbound27)
CSR_STRUCT_END(dlbound27)

CSR_STRUCT_START(dlbound28)
CSR_STRUCT_END(dlbound28)

CSR_STRUCT_START(dlbound29)
CSR_STRUCT_END(dlbound29)

CSR_STRUCT_START(dlbound30)
CSR_STRUCT_END(dlbound30)

CSR_STRUCT_START(dlbound31)
CSR_STRUCT_END(dlbound31)

CSR_STRUCT_START(dmaincall)
CSR_STRUCT_END(dmaincall)

CSR_STRUCT_START(dretpc)
CSR_STRUCT_END(dretpc)

CSR_STRUCT_START(dretpcfz)
CSR_STRUCT_END(dretpcfz)

CSR_STRUCT_START(djbound0lo)
CSR_STRUCT_END(djbound0lo)

CSR_STRUCT_START(djbound0hi)
CSR_STRUCT_END(djbound0hi)

CSR_STRUCT_START(djbound1lo)
CSR_STRUCT_END(djbound1lo)

CSR_STRUCT_START(djbound1hi)
CSR_STRUCT_END(djbound1hi)

CSR_STRUCT_START(djbound2lo)
CSR_STRUCT_END(djbound2lo)

CSR_STRUCT_START(djbound2hi)
CSR_STRUCT_END(djbound2hi)

CSR_STRUCT_START(djbound3lo)
CSR_STRUCT_END(djbound3lo)

CSR_STRUCT_START(djbound3hi)
CSR_STRUCT_END(djbound3hi)

CSR_STRUCT_START(djcfg)
CSR_STRUCT_END(djcfg)

#endif  // CONFIG_RV_DASICS

#define CSRS_DECL(name, addr) extern concat(name, _t)* const name;
MAP(CSRS, CSRS_DECL)
#ifdef CONFIG_RVN
  MAP(NCSRS, CSRS_DECL)
#endif
#ifdef CONFIG_RVV
  MAP(VCSRS, CSRS_DECL)
#endif // CONFIG_RVV
#ifdef CONFIG_RV_ARCH_CSRS
  MAP(ARCH_CSRS, CSRS_DECL)
#endif // CONFIG_RV_ARCH_CSRS
#ifdef CONFIG_RV_DASICS
  MAP(DASICS_CSRS, CSRS_DECL)
#endif // CONFIG_RV_DASICS

word_t csrid_read(uint32_t csrid);

// PMP
uint8_t pmpcfg_from_index(int idx);
word_t pmpaddr_from_index(int idx);
word_t pmpaddr_from_csrid(int id);
word_t pmp_tor_mask();

// DASICS
#ifdef CONFIG_RV_DASICS
bool dasics_in_trusted_zone(uint64_t pc);
uint8_t dasics_libcfg_from_index(int i);
word_t dasics_libbound_from_index(int i);
uint16_t dasics_jumpcfg_from_index(int i);
word_t dasics_jumpbound_low_from_index(int i);
word_t dasics_jumpbound_high_from_index(int i);
bool dasics_match_dlib(uint64_t addr, uint8_t cfg);
void dasics_ldst_helper(vaddr_t pc, vaddr_t vaddr, int len, int type);
void dasics_redirect_helper(vaddr_t pc, vaddr_t newpc, vaddr_t nextpc);
void dasics_check_trusted(vaddr_t pc);
#endif  // CONFIG_RV_DASICS

#endif
