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

#ifdef CONFIG_RVV_010
  #define VCSRS(f) \
  f(vstart, 0x008) \
  f(vxsat, 0x009) \
  f(vxrm, 0x00a) \
  f(vl, 0xc20) \
  f(vtype, 0xc21)
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

#ifdef CONFIG_RVV_010
// TODO: implement these vcsr
#define IDXVSTART 0x008
#define IDXVXSAT  0x009
#define IDXVXRM   0x00a
#define IDXVL     0xc20
#define IDXVTYPE  0xc21

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

CSR_STRUCT_START(vl)
CSR_STRUCT_END(vl)

CSR_STRUCT_START(vtype)
  uint64_t vlmul :  3;
  uint64_t vsew  :  3;
  uint64_t vediv :  2;
  uint64_t pad   : 55;
  uint64_t vill  :  1;
CSR_STRUCT_END(vtype)

rtlreg_t check_vsetvl(rtlreg_t vtype_req, rtlreg_t vl_req, bool max_req);
rtlreg_t get_mask(int reg, int idx, uint64_t vsew, uint64_t vlmul);
void set_mask(uint32_t reg, int idx, uint64_t mask, uint64_t vsew, uint64_t vlmul);

#endif // CONFIG_RVV_010

#ifdef CONFIG_RV_ARCH_CSRS
CSR_STRUCT_START(mvendorid)
CSR_STRUCT_END(mvendorid)

CSR_STRUCT_START(marchid)
CSR_STRUCT_END(marchid)

CSR_STRUCT_START(marchid)
CSR_STRUCT_END(mimpid)
#endif // CONFIG_RV_ARCH_CSRS

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
  union{
    struct{
      uint64_t pad5:11;
      uint64_t sd  : 1;
    }_32;
    struct{
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
#endif

#define CSRS_DECL(name, addr) extern concat(name, _t)* const name;
MAP(CSRS, CSRS_DECL)
#ifdef CONFIG_RVV_010
  MAP(VCSRS, CSRS_DECL)
#endif // CONFIG_RVV_010
#ifdef CONFIG_RV_ARCH_CSRS
  MAP(ARCH_CSRS, CSRS_DECL)
#endif // CONFIG_RV_ARCH_CSRS
#ifdef CONFIG_RVH
  extern bool v; // virtualization mode
  MAP(HCSRS, CSRS_DECL)
  #define vsatp_mode ((hstatus->vsxl == 1)? vsatp->_32.mode : vsatp->_64.mode)
  #define vsatp_asid ((hstatus->vsxl == 1)? vsatp->_32.asid : vsatp->_64.asid)
  #define vsatp_ppn  ((hstatus->vsxl == 1)? vsatp->_32.ppn  : vsatp->_64.ppn)
#endif
word_t csrid_read(uint32_t csrid);

// PMP
uint8_t pmpcfg_from_index(int idx);
word_t pmpaddr_from_index(int idx);
word_t pmpaddr_from_csrid(int id);
word_t pmp_tor_mask();

#ifdef CONFIG_RVH
#define CSR_VL 0xc20
#define CSR_VTYPE 0xc21
#define CSR_VLENB 0xc22
#define CSR_SSTATUS 0x100
#define CSR_SEDELEG 0x102
#define CSR_SIDELEG 0x103
#define CSR_SIE 0x104
#define CSR_STVEC 0x105
#define CSR_SCOUNTEREN 0x106
#define CSR_SENVCFG 0x10a
#define CSR_SSCRATCH 0x140
#define CSR_SEPC 0x141
#define CSR_SCAUSE 0x142
#define CSR_STVAL 0x143
#define CSR_SIP 0x144
#define CSR_SATP 0x180
#define CSR_SCONTEXT 0x5a8
#define CSR_VSSTATUS 0x200
#define CSR_VSIE 0x204
#define CSR_VSTVEC 0x205
#define CSR_VSSCRATCH 0x240
#define CSR_VSEPC 0x241
#define CSR_VSCAUSE 0x242
#define CSR_VSTVAL 0x243
#define CSR_VSIP 0x244
#define CSR_VSATP 0x280
#define CSR_HSTATUS 0x600
#define CSR_HEDELEG 0x602
#define CSR_HIDELEG 0x603
#define CSR_HIE 0x604
#define CSR_HTIMEDELTA 0x605
#define CSR_HCOUNTEREN 0x606
#define CSR_HGEIE 0x607
#define CSR_HENVCFG 0x60a
#define CSR_HTVAL 0x643
#define CSR_HIP 0x644
#define CSR_HVIP 0x645
#define CSR_HTINST 0x64a
#define CSR_HGATP 0x680
#define CSR_HCONTEXT 0x6a8
#define CSR_HGEIP 0xe12
#define CSR_UTVT 0x7
#define CSR_UNXTI 0x45
#define CSR_UINTSTATUS 0x46
#define CSR_USCRATCHCSW 0x48
#define CSR_USCRATCHCSWL 0x49
#define CSR_STVT 0x107
#define CSR_SNXTI 0x145
#define CSR_SINTSTATUS 0x146
#define CSR_SSCRATCHCSW 0x148
#define CSR_SSCRATCHCSWL 0x149
#define CSR_MTVT 0x307
#define CSR_MNXTI 0x345
#define CSR_MINTSTATUS 0x346
#define CSR_MSCRATCHCSW 0x348
#define CSR_MSCRATCHCSWL 0x349
#define CSR_MSTATUS 0x300
#define CSR_MISA 0x301
#define CSR_MEDELEG 0x302
#define CSR_MIDELEG 0x303
#define CSR_MIE 0x304
#define CSR_MTVEC 0x305
#define CSR_MCOUNTEREN 0x306
#define CSR_MENVCFG 0x30a
#define CSR_MCOUNTINHIBIT 0x320
#define CSR_MSCRATCH 0x340
#define CSR_MEPC 0x341
#define CSR_MCAUSE 0x342
#define CSR_MTVAL 0x343
#define CSR_MIP 0x344
#define CSR_MTINST 0x34a
#define CSR_MTVAL2 0x34b
#define CSR_PMPCFG0 0x3a0
#define CSR_PMPCFG1 0x3a1
#define CSR_PMPCFG2 0x3a2
#define CSR_PMPCFG3 0x3a3
#define CSR_PMPCFG4 0x3a4
#define CSR_PMPCFG5 0x3a5
#define CSR_PMPCFG6 0x3a6
#define CSR_PMPCFG7 0x3a7
#define CSR_PMPCFG8 0x3a8
#define CSR_PMPCFG9 0x3a9
#define CSR_PMPCFG10 0x3aa
#define CSR_PMPCFG11 0x3ab
#define CSR_PMPCFG12 0x3ac
#define CSR_PMPCFG13 0x3ad
#define CSR_PMPCFG14 0x3ae
#define CSR_PMPCFG15 0x3af
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
#define CSR_PMPADDR16 0x3c0
#define CSR_PMPADDR17 0x3c1
#define CSR_PMPADDR18 0x3c2
#define CSR_PMPADDR19 0x3c3
#define CSR_PMPADDR20 0x3c4
#define CSR_PMPADDR21 0x3c5
#define CSR_PMPADDR22 0x3c6
#define CSR_PMPADDR23 0x3c7
#define CSR_PMPADDR24 0x3c8
#define CSR_PMPADDR25 0x3c9
#define CSR_PMPADDR26 0x3ca
#define CSR_PMPADDR27 0x3cb
#define CSR_PMPADDR28 0x3cc
#define CSR_PMPADDR29 0x3cd
#define CSR_PMPADDR30 0x3ce
#define CSR_PMPADDR31 0x3cf
#define CSR_PMPADDR32 0x3d0
#define CSR_PMPADDR33 0x3d1
#define CSR_PMPADDR34 0x3d2
#define CSR_PMPADDR35 0x3d3
#define CSR_PMPADDR36 0x3d4
#define CSR_PMPADDR37 0x3d5
#define CSR_PMPADDR38 0x3d6
#define CSR_PMPADDR39 0x3d7
#define CSR_PMPADDR40 0x3d8
#define CSR_PMPADDR41 0x3d9
#define CSR_PMPADDR42 0x3da
#define CSR_PMPADDR43 0x3db
#define CSR_PMPADDR44 0x3dc
#define CSR_PMPADDR45 0x3dd
#define CSR_PMPADDR46 0x3de
#define CSR_PMPADDR47 0x3df
#define CSR_PMPADDR48 0x3e0
#define CSR_PMPADDR49 0x3e1
#define CSR_PMPADDR50 0x3e2
#define CSR_PMPADDR51 0x3e3
#define CSR_PMPADDR52 0x3e4
#define CSR_PMPADDR53 0x3e5
#define CSR_PMPADDR54 0x3e6
#define CSR_PMPADDR55 0x3e7
#define CSR_PMPADDR56 0x3e8
#define CSR_PMPADDR57 0x3e9
#define CSR_PMPADDR58 0x3ea
#define CSR_PMPADDR59 0x3eb
#define CSR_PMPADDR60 0x3ec
#define CSR_PMPADDR61 0x3ed
#define CSR_PMPADDR62 0x3ee
#define CSR_PMPADDR63 0x3ef
#define CSR_MSECCFG 0x747
#define CSR_TSELECT 0x7a0
#define CSR_TDATA1 0x7a1
#define CSR_TDATA2 0x7a2
#define CSR_TDATA3 0x7a3
#define CSR_TINFO 0x7a4
#define CSR_TCONTROL 0x7a5
#define CSR_MCONTEXT 0x7a8
#define CSR_MSCONTEXT 0x7aa
#define CSR_DCSR 0x7b0
#define CSR_DPC 0x7b1
#define CSR_DSCRATCH0 0x7b2
#define CSR_DSCRATCH1 0x7b3
#define CSR_MCYCLE 0xb00
#define CSR_MINSTRET 0xb02
#define CSR_MHPMCOUNTER3 0xb03
#define CSR_MHPMCOUNTER4 0xb04
#define CSR_MHPMCOUNTER5 0xb05
#define CSR_MHPMCOUNTER6 0xb06
#define CSR_MHPMCOUNTER7 0xb07
#define CSR_MHPMCOUNTER8 0xb08
#define CSR_MHPMCOUNTER9 0xb09
#define CSR_MHPMCOUNTER10 0xb0a
#define CSR_MHPMCOUNTER11 0xb0b
#define CSR_MHPMCOUNTER12 0xb0c
#define CSR_MHPMCOUNTER13 0xb0d
#define CSR_MHPMCOUNTER14 0xb0e
#define CSR_MHPMCOUNTER15 0xb0f
#define CSR_MHPMCOUNTER16 0xb10
#define CSR_MHPMCOUNTER17 0xb11
#define CSR_MHPMCOUNTER18 0xb12
#define CSR_MHPMCOUNTER19 0xb13
#define CSR_MHPMCOUNTER20 0xb14
#define CSR_MHPMCOUNTER21 0xb15
#define CSR_MHPMCOUNTER22 0xb16
#define CSR_MHPMCOUNTER23 0xb17
#define CSR_MHPMCOUNTER24 0xb18
#define CSR_MHPMCOUNTER25 0xb19
#define CSR_MHPMCOUNTER26 0xb1a
#define CSR_MHPMCOUNTER27 0xb1b
#define CSR_MHPMCOUNTER28 0xb1c
#define CSR_MHPMCOUNTER29 0xb1d
#define CSR_MHPMCOUNTER30 0xb1e
#define CSR_MHPMCOUNTER31 0xb1f
#define CSR_MHPMEVENT3 0x323
#define CSR_MHPMEVENT4 0x324
#define CSR_MHPMEVENT5 0x325
#define CSR_MHPMEVENT6 0x326
#define CSR_MHPMEVENT7 0x327
#define CSR_MHPMEVENT8 0x328
#define CSR_MHPMEVENT9 0x329
#define CSR_MHPMEVENT10 0x32a
#define CSR_MHPMEVENT11 0x32b
#define CSR_MHPMEVENT12 0x32c
#define CSR_MHPMEVENT13 0x32d
#define CSR_MHPMEVENT14 0x32e
#define CSR_MHPMEVENT15 0x32f
#define CSR_MHPMEVENT16 0x330
#define CSR_MHPMEVENT17 0x331
#define CSR_MHPMEVENT18 0x332
#define CSR_MHPMEVENT19 0x333
#define CSR_MHPMEVENT20 0x334
#define CSR_MHPMEVENT21 0x335
#define CSR_MHPMEVENT22 0x336
#define CSR_MHPMEVENT23 0x337
#define CSR_MHPMEVENT24 0x338
#define CSR_MHPMEVENT25 0x339
#define CSR_MHPMEVENT26 0x33a
#define CSR_MHPMEVENT27 0x33b
#define CSR_MHPMEVENT28 0x33c
#define CSR_MHPMEVENT29 0x33d
#define CSR_MHPMEVENT30 0x33e
#define CSR_MHPMEVENT31 0x33f
#define CSR_MVENDORID 0xf11
#define CSR_MARCHID 0xf12
#define CSR_MIMPID 0xf13
#define CSR_MHARTID 0xf14
#define CSR_MCONFIGPTR 0xf15
#define CSR_HTIMEDELTAH 0x615
#define CSR_HENVCFGH 0x61a
#define CSR_CYCLEH 0xc80
#define CSR_TIMEH 0xc81
#define CSR_INSTRETH 0xc82
#define CSR_HPMCOUNTER3H 0xc83
#define CSR_HPMCOUNTER4H 0xc84
#define CSR_HPMCOUNTER5H 0xc85
#define CSR_HPMCOUNTER6H 0xc86
#define CSR_HPMCOUNTER7H 0xc87
#define CSR_HPMCOUNTER8H 0xc88
#define CSR_HPMCOUNTER9H 0xc89
#define CSR_HPMCOUNTER10H 0xc8a
#define CSR_HPMCOUNTER11H 0xc8b
#define CSR_HPMCOUNTER12H 0xc8c
#define CSR_HPMCOUNTER13H 0xc8d
#define CSR_HPMCOUNTER14H 0xc8e
#define CSR_HPMCOUNTER15H 0xc8f
#define CSR_HPMCOUNTER16H 0xc90
#define CSR_HPMCOUNTER17H 0xc91
#define CSR_HPMCOUNTER18H 0xc92
#define CSR_HPMCOUNTER19H 0xc93
#define CSR_HPMCOUNTER20H 0xc94
#define CSR_HPMCOUNTER21H 0xc95
#define CSR_HPMCOUNTER22H 0xc96
#define CSR_HPMCOUNTER23H 0xc97
#define CSR_HPMCOUNTER24H 0xc98
#define CSR_HPMCOUNTER25H 0xc99
#define CSR_HPMCOUNTER26H 0xc9a
#define CSR_HPMCOUNTER27H 0xc9b
#define CSR_HPMCOUNTER28H 0xc9c
#define CSR_HPMCOUNTER29H 0xc9d
#define CSR_HPMCOUNTER30H 0xc9e
#define CSR_HPMCOUNTER31H 0xc9f
#define CSR_MSTATUSH 0x310
#define CSR_MENVCFGH 0x31a
#define CSR_MSECCFGH 0x757
#define CSR_MCYCLEH 0xb80
#define CSR_MINSTRETH 0xb82
#define CSR_MHPMCOUNTER3H 0xb83
#define CSR_MHPMCOUNTER4H 0xb84
#define CSR_MHPMCOUNTER5H 0xb85
#define CSR_MHPMCOUNTER6H 0xb86
#define CSR_MHPMCOUNTER7H 0xb87
#define CSR_MHPMCOUNTER8H 0xb88
#define CSR_MHPMCOUNTER9H 0xb89
#define CSR_MHPMCOUNTER10H 0xb8a
#define CSR_MHPMCOUNTER11H 0xb8b
#define CSR_MHPMCOUNTER12H 0xb8c
#define CSR_MHPMCOUNTER13H 0xb8d
#define CSR_MHPMCOUNTER14H 0xb8e
#define CSR_MHPMCOUNTER15H 0xb8f
#define CSR_MHPMCOUNTER16H 0xb90
#define CSR_MHPMCOUNTER17H 0xb91
#define CSR_MHPMCOUNTER18H 0xb92
#define CSR_MHPMCOUNTER19H 0xb93
#define CSR_MHPMCOUNTER20H 0xb94
#define CSR_MHPMCOUNTER21H 0xb95
#define CSR_MHPMCOUNTER22H 0xb96
#define CSR_MHPMCOUNTER23H 0xb97
#define CSR_MHPMCOUNTER24H 0xb98
#define CSR_MHPMCOUNTER25H 0xb99
#define CSR_MHPMCOUNTER26H 0xb9a
#define CSR_MHPMCOUNTER27H 0xb9b
#define CSR_MHPMCOUNTER28H 0xb9c
#define CSR_MHPMCOUNTER29H 0xb9d
#define CSR_MHPMCOUNTER30H 0xb9e
#define CSR_MHPMCOUNTER31H 0xb9f
#endif
#endif
