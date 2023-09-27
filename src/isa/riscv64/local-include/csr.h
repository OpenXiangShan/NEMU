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

// Debug Mode ISA CSRs in Sdext ISA extension
#ifdef CONFIG_RVSDEXT
  #define CORE_DEBUG_CSRS(f) \
  f(dcsr       , 0x7b0) f(dpc        , 0x7b1) f(dscratch0  , 0x7b2) f(dscratch1  , 0x7b3) \

#else
  #define CORE_DEBUG_CSRS(f)
#endif // CONFIG_RVSDEXT

// Trigger CSRs in Sdtrig ISA extension
#ifdef CONFIG_RVSDTRIG
  #define TRIGGER_CSRS(f) \
  f(scontext   , 0x6a8) \
  f(tselect    , 0x7a0) f(tdata1     , 0x7a1) f(tdata2     , 0x7a2) f(tdata3     , 0x7a3) \
  f(tinfo      , 0x7a4) f(tcontrol   , 0x7a5) f(mcontext   , 0x7a8) \

#else
  #define TRIGGER_CSRS(f)
#endif // CONFIG_RVSDTRIG

void csr_prepare();

#define CSRS_HPM(f) \
  f(mhpmcounter3   , 0xB03) f(mhpmcounter4   , 0xB04) f(mhpmcounter5   , 0xB05) f(mhpmcounter6   , 0xB06) \
  f(mhpmcounter7   , 0xB07) f(mhpmcounter8   , 0xB08) f(mhpmcounter9   , 0xB09) f(mhpmcounter10  , 0xB0a) \
  f(mhpmcounter11  , 0xB0b) f(mhpmcounter12  , 0xB0c) f(mhpmcounter13  , 0xB0d) f(mhpmcounter14  , 0xB0e) \
  f(mhpmcounter15  , 0xB0f) f(mhpmcounter16  , 0xB10) f(mhpmcounter17  , 0xB11) f(mhpmcounter18  , 0xB12) \
  f(mhpmcounter19  , 0xB13) f(mhpmcounter20  , 0xB14) f(mhpmcounter21  , 0xB15) f(mhpmcounter22  , 0xB16) \
  f(mhpmcounter23  , 0xB17) f(mhpmcounter24  , 0xB18) f(mhpmcounter25  , 0xB19) f(mhpmcounter26  , 0xB1a) \
  f(mhpmcounter27  , 0xB1b) f(mhpmcounter28  , 0xB1c) f(mhpmcounter29  , 0xB1d) f(mhpmcounter30  , 0xB1e) \
  f(mhpmcounter31  , 0xB1f) \
  f(mcountinhibit  , 0x320) \
  f(mhpmevent3     , 0x323) f(mhpmevent4     , 0x324) f(mhpmevent5     , 0x325) f(mhpmevent6     , 0x326) \
  f(mhpmevent7     , 0x327) f(mhpmevent8     , 0x328) f(mhpmevent9     , 0x329) f(mhpmevent10    , 0x32a) \
  f(mhpmevent11    , 0x32b) f(mhpmevent12    , 0x32c) f(mhpmevent13    , 0x32d) f(mhpmevent14    , 0x32e) \
  f(mhpmevent15    , 0x32f) f(mhpmevent16    , 0x330) f(mhpmevent17    , 0x331) f(mhpmevent18    , 0x332) \
  f(mhpmevent19    , 0x333) f(mhpmevent20    , 0x334) f(mhpmevent21    , 0x335) f(mhpmevent22    , 0x336) \
  f(mhpmevent23    , 0x337) f(mhpmevent24    , 0x338) f(mhpmevent25    , 0x339) f(mhpmevent26    , 0x33a) \
  f(mhpmevent27    , 0x33b) f(mhpmevent28    , 0x33c) f(mhpmevent29    , 0x33d) f(mhpmevent30    , 0x3e) \
  f(mhpmeven31     , 0x33f)
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

#ifndef CONFIG_SHARE
#define CSRS(f) \
  f(mstatus    , 0x300) f(misa       , 0x301) f(medeleg    , 0x302) f(mideleg    , 0x303) \
  f(mie        , 0x304) f(mtvec      , 0x305) f(mcounteren , 0x306) \
  f(mscratch   , 0x340) f(mepc       , 0x341) f(mcause     , 0x342) \
  f(mtval      , 0x343) f(mip        , 0x344) \
  f(menvcfg    , 0x30A) \
  CSRS_PMP(f) \
  f(mhartid    , 0xf14) \
  f(sstatus    , 0x100) \
  f(sie        , 0x104) f(stvec      , 0x105) f(scounteren , 0x106) \
  f(sscratch   , 0x140) f(sepc       , 0x141) f(scause     , 0x142) \
  f(stval      , 0x143) f(sip        , 0x144) \
  f(satp       , 0x180) \
  f(mcycle     , 0xb00) f(minstret   , 0xb02) \
  CUSTOM_CSR(f) \
  f(fflags     , 0x001) f(frm        , 0x002) f(fcsr       , 0x003) \
  f(mtime      , 0xc01) \
  CORE_DEBUG_CSRS(f) \
  TRIGGER_CSRS(f) \

#else
#define CSRS(f) \
  f(mstatus    , 0x300) f(misa       , 0x301) f(medeleg    , 0x302) f(mideleg    , 0x303) \
  f(mie        , 0x304) f(mtvec      , 0x305) f(mcounteren , 0x306) \
  f(mscratch   , 0x340) f(mepc       , 0x341) f(mcause     , 0x342) \
  f(mtval      , 0x343) f(mip        , 0x344) \
  f(menvcfg    , 0x30A) \
  CSRS_PMP(f) \
  f(mhartid    , 0xf14) \
  f(sstatus    , 0x100) \
  f(sie        , 0x104) f(stvec      , 0x105) f(scounteren , 0x106) \
  f(sscratch   , 0x140) f(sepc       , 0x141) f(scause     , 0x142) \
  f(stval      , 0x143) f(sip        , 0x144) \
  f(satp       , 0x180) \
  f(mcycle     , 0xb00) f(minstret   , 0xb02) \
  CUSTOM_CSR(f) \
  f(fflags     , 0x001) f(frm        , 0x002) f(fcsr       , 0x003) \
  CORE_DEBUG_CSRS(f) \
  TRIGGER_CSRS(f) \

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

#ifdef CONFIG_RVH
  #define HCSRS(f) \
  f(hstatus    , 0x600) f(hedeleg    , 0x602) f(hideleg    , 0x603) \
  f(hie        , 0x604) f(hcounteren , 0x606) f(hgeie      , 0x607) \
  f(htval      , 0x643) f(hip        , 0x644) f(hvip       , 0x645) \
  f(htinst     , 0x64A) f(hgeip      , 0xE12) f(henvcfg    , 0x60A) \
  f(hgatp      , 0x680) f(htimedelta , 0x605) \
  f(vsstatus   , 0x200) f(vsie       , 0x204) f(vstvec     , 0x205) \
  f(vsscratch  , 0x240) f(vsepc      , 0x241) f(vscause    , 0x242) \
  f(vstval     , 0x243) f(vsip       , 0x244) f(vsatp      , 0x280) \
  f(mtval2     , 0x34b) f(mtinst     , 0x34A)
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
#ifdef CONFIG_RVH
  uint64_t gva : 1;
  uint64_t mpv : 1;
  uint64_t pad4:23;
#else
  uint64_t pad4:25;
#endif
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

CSR_STRUCT_START(menvcfg)
CSR_STRUCT_END(menvcfg)

#ifdef CONFIG_RVH
CSR_STRUCT_START(mtval2)
CSR_STRUCT_END(mtval2)

CSR_STRUCT_START(mtinst)
CSR_STRUCT_END(mtinst)
#endif

CSR_STRUCT_START(mie)
  uint64_t usie : 1;
  uint64_t ssie : 1;
  uint64_t vssie: 1;
  uint64_t msie : 1;
  uint64_t utie : 1;
  uint64_t stie : 1;
  uint64_t vstie: 1;
  uint64_t mtie : 1;
  uint64_t ueie : 1;
  uint64_t seie : 1;
  uint64_t vseie: 1;
  uint64_t meie : 1;
  uint64_t sgeie: 1;
CSR_STRUCT_END(mie)

CSR_STRUCT_START(mip)
  uint64_t usip : 1;
  uint64_t ssip : 1;
  uint64_t vssip: 1;
  uint64_t msip : 1;
  uint64_t utip : 1;
  uint64_t stip : 1;
  uint64_t vstip: 1;
  uint64_t mtip : 1;
  uint64_t ueip : 1;
  uint64_t seip : 1;
  uint64_t vseip: 1;
  uint64_t meip : 1;
  uint64_t sgeip: 1;
CSR_STRUCT_END(mip)

/** pmp */

#define PMP_R     0x01
#define PMP_W     0x02
#define PMP_X     0x04
#define PMP_A     0x18
#define PMP_L     0x80
#define PMP_SHIFT 2

#ifdef CONFIG_PMPTABLE_EXTENSION
#define PMP_T     0x20
#define PMP_C     0x40
#endif

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

#define HGATP_VMID_LEN 14 // max is 14
#define HGATP_PADDR_LEN 44 // max is 44
#define HGATP_VMID_MAX_LEN 16
#define HGATP_PADDR_MAX_LEN 44

#define HGATP_MODE_MASK (8UL << (HGATP_VMID_MAX_LEN + HGATP_PADDR_MAX_LEN))
#define HGATP_VMID_MASK (((1L << HGATP_VMID_LEN)-1) << HGATP_PADDR_MAX_LEN)
#define HGATP_PADDR_MASK ((1L << HGATP_PADDR_MAX_LEN)-1)

#define HGATP_MASK (HGATP_MODE_MASK | HGATP_VMID_MASK | HGATP_PADDR_MASK)
#define MASKED_HGATP(x) (HGATP_MASK & x)

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

CSR_STRUCT_START(mcycle)
CSR_STRUCT_END(mcycle)

CSR_STRUCT_START(minstret)
CSR_STRUCT_END(minstret)

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
  uint64_t vta   :  1;
  uint64_t vma   :  1;
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

CSR_STRUCT_START(mimpid)
CSR_STRUCT_END(mimpid)
#endif // CONFIG_RV_ARCH_CSRS

#ifdef CONFIG_RVSDEXT
CSR_STRUCT_START(dcsr)
  uint64_t prv      : 2 ; // [1:0]
  uint64_t step     : 1 ; // [2]
  uint64_t nmip     : 1 ; // [3]
  uint64_t mprven   : 1 ; // [4]
  uint64_t v        : 1 ; // [5]
  uint64_t cause    : 3 ; // [8:6]
  uint64_t stoptime : 1 ; // [9]
  uint64_t stopcount: 1 ; // [10]
  uint64_t stepie   : 1 ; // [11]
  uint64_t ebreaku  : 1 ; // [12]
  uint64_t ebreaks  : 1 ; // [13]
  uint64_t pad0     : 1 ; // [14]
  uint64_t ebreakm  : 1 ; // [15]
  uint64_t ebreakvu : 1 ; // [16]
  uint64_t ebreakvs : 1 ; // [17]
  uint64_t pad1     : 10; // [27:18]
  uint64_t debugver : 4 ; // [31:28]
CSR_STRUCT_END(dcsr)

CSR_STRUCT_START(dpc)
CSR_STRUCT_END(dpc)

CSR_STRUCT_START(dscratch0)
CSR_STRUCT_END(dscratch0)

CSR_STRUCT_START(dscratch1)
CSR_STRUCT_END(dscratch1)
#endif // CONFIG_RVSDEXT

#ifdef CONFIG_RVSDTRIG
CSR_STRUCT_START(scontext)  // 0x5a8
CSR_STRUCT_END(scontext)

CSR_STRUCT_START(tselect)   // 0x7a0
CSR_STRUCT_END(tselect)

CSR_STRUCT_START(tdata1)    // 0x7a1
  uint64_t data   : 59;     // [58:0]
  uint64_t dmode  : 1;      // [59]
  uint64_t type   : 4;      // [63:60]
CSR_STRUCT_END(tdata1)

CSR_STRUCT_START(tdata2)    // 0x7a2
CSR_STRUCT_END(tdata2)

CSR_STRUCT_START(tdata3)    // 0x7a3
  union {
    struct {
      uint64_t sselect    : 2;  // [1:0]
      uint64_t svalue     : 34; // [35:2]
      uint64_t sbytemask  : 5;  // [40:36]
      uint64_t pad0       : 7;  // [47:41]
      uint64_t mhselect   : 3;  // [50:48]
      uint64_t mhvalue    : 13; // [63:51]
    } textra64;
  };
CSR_STRUCT_END(tdata3)

CSR_STRUCT_START(tinfo)     // 0x7a4
  uint64_t info : 16;       // [15:0]
CSR_STRUCT_END(tinfo)

CSR_STRUCT_START(tcontrol)  // 0x7a5
  uint64_t pad0 : 3;        // [2:0]
  uint64_t mte  : 1;        // [3]
  uint64_t pad1 : 3;        // [6:4]
  uint64_t mpte : 1;        // [7]
CSR_STRUCT_END(tcontrol)

CSR_STRUCT_START(mcontext)  // 0x7a8
CSR_STRUCT_END(mcontext)

#endif // CONFIG_RVSDTRIG

#ifdef CONFIG_RVH
CSR_STRUCT_START(hstatus)
  uint64_t pad0  : 5;
  uint64_t vsbe  : 1;
  uint64_t gva   : 1;
  uint64_t spv   : 1;
  uint64_t spvp  : 1;
  uint64_t hu    : 1;
  uint64_t pad1  : 2;
  uint64_t vgein : 6;
  uint64_t pad2  : 2;
  uint64_t vtvm  : 1;
  uint64_t vtw   : 1;
  uint64_t vtsr  : 1;
  uint64_t pad3  : 9;
  uint64_t vsxl  : 2;
CSR_STRUCT_END(hstatus)

CSR_STRUCT_START(hedeleg)
CSR_STRUCT_END(hedeleg)

CSR_STRUCT_START(hideleg)
CSR_STRUCT_END(hideleg)

CSR_STRUCT_START(hie)
  uint64_t pad0  : 2;
  uint64_t vssie : 1;
  uint64_t pad1  : 3;
  uint64_t vstie : 1;
  uint64_t pad2  : 3;
  uint64_t vseie : 1;
  uint64_t pad3  : 1;
  uint64_t sgeie : 1;
CSR_STRUCT_END(hie)

CSR_STRUCT_START(hcounteren)
CSR_STRUCT_END(hcounteren)

CSR_STRUCT_START(hgeie)
CSR_STRUCT_END(hgeie)

CSR_STRUCT_START(htval)
CSR_STRUCT_END(htval)

CSR_STRUCT_START(hip)
  uint64_t pad0  : 2;
  uint64_t vssip : 1;
  uint64_t pad1  : 3;
  uint64_t vstip : 1;
  uint64_t pad2  : 3;
  uint64_t vseip : 1;
  uint64_t pad3  : 1;
  uint64_t sgeip : 1;
CSR_STRUCT_END(hip)

CSR_STRUCT_START(hvip)
  uint64_t pad0  : 2;
  uint64_t vssip : 1;
  uint64_t pad1  : 3;
  uint64_t vstip : 1;
  uint64_t pad2  : 3;
  uint64_t vseip : 1;
CSR_STRUCT_END(hvip)

CSR_STRUCT_START(htinst)
CSR_STRUCT_END(htinst)

CSR_STRUCT_START(hgeip)
CSR_STRUCT_END(hgeip)

CSR_STRUCT_START(henvcfg)
  uint64_t fiom   : 1;
  uint64_t pad0   : 3;
  uint64_t cbie   : 2;
  uint64_t cbcfe  : 1;
  uint64_t cbze   : 1;
  uint64_t pad1   :54;
  uint64_t pbmte  : 1;
  uint64_t vstce  : 1;
CSR_STRUCT_END(henvcfg)

CSR_STRUCT_START(hgatp)
  uint64_t ppn    : 44;
  uint64_t vmid   : 14;
  uint64_t pad0   : 2;
  uint64_t mode   : 4;
CSR_STRUCT_END(hgatp)

CSR_STRUCT_START(htimedelta)
CSR_STRUCT_END(htimedelta)

CSR_STRUCT_START(vsstatus)
  union{
    struct{
      uint64_t pad0: 1;
      uint64_t sie : 1;
      uint64_t pad1: 3;
      uint64_t spie: 1;
      uint64_t ube : 1;
      uint64_t pad2: 1;
      uint64_t spp : 1;
      uint64_t vs  : 2;
      uint64_t pad3: 2;
      uint64_t fs  : 2;
      uint64_t xs  : 2;
      uint64_t pad4: 1;
      uint64_t sum : 1;
      uint64_t mxr : 1;
      uint64_t pad5:11;
      uint64_t sd  : 1;
    }_32;
    struct{
      uint64_t pad0: 1;
      uint64_t sie : 1;
      uint64_t pad1: 3;
      uint64_t spie: 1;
      uint64_t ube : 1;
      uint64_t pad2: 1;
      uint64_t spp : 1;
      uint64_t vs  : 2;
      uint64_t pad3: 2;
      uint64_t fs  : 2;
      uint64_t xs  : 2;
      uint64_t pad4: 1;
      uint64_t sum : 1;
      uint64_t mxr : 1;
      uint64_t pad5:12;
      uint64_t uxl : 2;
      uint64_t pad6:29;
      uint64_t sd  : 1;
    }_64;
  };
CSR_STRUCT_END(vsstatus)

CSR_STRUCT_START(vsie)
  uint64_t pad0 : 1;
  uint64_t ssie : 1;
  uint64_t pad1 : 3;
  uint64_t stie : 1;
  uint64_t pad2 : 3;
  uint64_t seie : 1;
CSR_STRUCT_END(vsie)

CSR_STRUCT_START(vstvec)
  uint64_t mode  : 2;
  uint64_t base  :62;
CSR_STRUCT_END(vstvec)

CSR_STRUCT_START(vsscratch)
CSR_STRUCT_END(vsscratch)

CSR_STRUCT_START(vsepc)
CSR_STRUCT_END(vsepc)

CSR_STRUCT_START(vscause)
  union{
    struct{
      uint64_t code:31;
      uint64_t intr: 1;
    }_32;
    struct{
      uint64_t code:63;
      uint64_t intr: 1;
    }_64;
  };
CSR_STRUCT_END(vscause)

CSR_STRUCT_START(vstval)
CSR_STRUCT_END(vstval)

CSR_STRUCT_START(vsip)
  uint64_t pad0 : 1;
  uint64_t ssip : 1;
  uint64_t pad1 : 3;
  uint64_t stip : 1;
  uint64_t pad2 : 3;
  uint64_t seip : 1;
CSR_STRUCT_END(vsip)

CSR_STRUCT_START(vsatp)
  union{
    struct{
      uint64_t ppn  :22;
      uint64_t asid : 9;
      uint64_t mode : 1;
    }_32;
    struct{
      uint64_t ppn  :44;
      uint64_t asid :16;
      uint64_t mode : 4;
    }_64;
  };
CSR_STRUCT_END(vsatp)
#endif //CONFIG_RVH

#define CSRS_DECL(name, addr) extern concat(name, _t)* const name;
MAP(CSRS, CSRS_DECL)
#ifdef CONFIG_RVV
  MAP(VCSRS, CSRS_DECL)
#endif // CONFIG_RVV
#ifdef CONFIG_RV_ARCH_CSRS
  MAP(ARCH_CSRS, CSRS_DECL)
#endif // CONFIG_RV_ARCH_CSRS
#ifdef CONFIG_RVH
  extern bool v; // virtualization mode
  MAP(HCSRS, CSRS_DECL)
  #define vsatp_mode ((hstatus->vsxl == 1)? vsatp->_32.mode : vsatp->_64.mode)
  #define vsatp_asid ((hstatus->vsxl == 1)? vsatp->_32.asid : vsatp->_64.asid)
  #define vsatp_ppn  ((hstatus->vsxl == 1)? vsatp->_32.ppn  : vsatp->_64.ppn)
  #define _vsstatus_  ((hstatus->vsxl == 1)? vsstatus->_32  : vsstatus->_64)
#endif // CONFIG_RVH

 #ifdef CONFIG_RVV
  #define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (0x3 << 9) | (1 << 8) | (1 << 5) | (1 << 1))
#else
  #define SSTATUS_WMASK ((1 << 19) | (1 << 18) | (0x3 << 13) | (1 << 8) | (1 << 5) | (1 << 1))
#endif // CONFIG_RVV
#define SSTATUS_RMASK (SSTATUS_WMASK | (0x3 << 15) | (1ull << 63) | (3ull << 32))
word_t csrid_read(uint32_t csrid);

// PMP
uint8_t pmpcfg_from_index(int idx);
word_t pmpaddr_from_index(int idx);
word_t pmpaddr_from_csrid(int id);
word_t pmp_tor_mask();

#endif
