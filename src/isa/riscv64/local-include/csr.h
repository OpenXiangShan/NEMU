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
#include <memory/vaddr.h>

/**
 * Mapping between CSR name and addr
 * 
 * This part follows the order of "CSR Listing" section in Privileged ISA Manual.
*/

/* Unprivileged CSR */
/** Unprivileged Floating-Point CSRs **/
#ifndef CONFIG_FPU_NONE
  #define CSRS_UNPRIV_FLOAT(f) \
    f(fflags       , 0x001) f(frm        , 0x002) f(fcsr       , 0x003)
#else // CONFIG_FPU_NONE
  #define CSRS_UNPRIV_FLOAT(f)
#endif // CONFIG_FPU_NONE

/** Unprivileged Counter/Timers **/
#ifdef CONFIG_RV_CSR_TIME
  #define CSRS_UNPRIV_TIME(f) \
    f(csr_time   , 0xC01)
#else // CONFIG_RV_CSR_TIME
  #define CSRS_UNPRIV_TIME(f)
#endif // CONFIG_RV_CSR_TIME

#ifdef CONFIG_RV_Zicntr
  #define CSRS_UNPRIV_CNTR(f) \
    f(cycle      , 0xC00) \
    CSRS_UNPRIV_TIME(f) \
    f(instret    , 0xC02)
    // There is `time_t` type in the C programming language.
    // So We have to use another name for CSR time.
#else // CONFIG_RV_Zicntr
  #define CSRS_UNPRIV_CNTR(f)
#endif // CONFIG_RV_Zicntr

#ifdef CONFIG_RV_Zihpm
  #define CSRS_UNPRIV_HPMCOUNTER(f) \
    f(hpmcounter3    , 0xC03) \
    f(hpmcounter4    , 0xC04) f(hpmcounter5    , 0xC05) f(hpmcounter6    , 0xC06) f(hpmcounter7    , 0xC07) \
    f(hpmcounter8    , 0xC08) f(hpmcounter9    , 0xC09) f(hpmcounter10   , 0xC0A) f(hpmcounter11   , 0xC0B) \
    f(hpmcounter12   , 0xC0C) f(hpmcounter13   , 0xC0D) f(hpmcounter14   , 0xC0E) f(hpmcounter15   , 0xC0F) \
    f(hpmcounter16   , 0xC10) f(hpmcounter17   , 0xC11) f(hpmcounter18   , 0xC12) f(hpmcounter19   , 0xC13) \
    f(hpmcounter20   , 0xC14) f(hpmcounter21   , 0xC15) f(hpmcounter22   , 0xC16) f(hpmcounter23   , 0xC17) \
    f(hpmcounter24   , 0xC18) f(hpmcounter25   , 0xC19) f(hpmcounter26   , 0xC1A) f(hpmcounter27   , 0xC1B) \
    f(hpmcounter28   , 0xC1C) f(hpmcounter29   , 0xC1D) f(hpmcounter30   , 0xC1E) f(hpmcounter31   , 0xC1F)
#else // CONFIG_RV_Zihpm
  #define CSRS_UNPRIV_HPMCOUNTER(f)
#endif // CONFIG_RV_Zihpm

#define CSRS_UNPRIV_COUNTER_TIMERS(f) \
  CSRS_UNPRIV_CNTR(f) \
  CSRS_UNPRIV_HPMCOUNTER(f)

/** Unprivileged Vector CSRs **/
#ifdef CONFIG_RVV
  #define CSRS_UNPRIV_VECTOR(f) \
    f(vstart     , 0x008) f(vxsat      , 0x009) f(vxrm       , 0x00A) \
    f(vcsr       , 0x00F) \
    f(vl         , 0xC20) f(vtype      , 0xC21) f(vlenb      , 0xC22)
#else // CONFIG_RVV
  #define CSRS_UNPRIV_VECTOR(f)
#endif // CONFIG_RVV

/** ALL **/
#define CSRS_UNPRIV(f) \
  CSRS_UNPRIV_FLOAT(f) \
  CSRS_UNPRIV_COUNTER_TIMERS(f) \
  CSRS_UNPRIV_VECTOR(f)


/* Supervisor-level CSR */
/** Supervisor Trap Setup **/
#define CSRS_S_TRAP_SETUP(f) \
  f(sstatus    , 0x100) f(sie        , 0x104) f(stvec      , 0x105) \
  f(scounteren , 0x106)

/** Supervisor Configuration **/
#define CSRS_S_CONFIGURATION(f) \
  f(senvcfg    , 0x10A)

/** Supervisor Counter Setup **/
#ifdef CONFIG_RV_Smcdeleg
  #define CSRS_S_COUNTER_SETUP(f) \
    f(scountinhibit, 0x120)
#else // CONFIG_RV_Smcdeleg
  #define CSRS_S_COUNTER_SETUP(f)
#endif // CONFIG_RV_Smcdeleg

/** Supervisor Trap Handling **/
#define CSRS_S_TRAP_HANDLING(f) \
  f(sscratch   , 0x140) f(sepc       , 0x141) f(scause     , 0x142) \
  f(stval      , 0x143) f(sip        , 0x144)

/** Supervisor Protection and Translation **/
#define CSRS_S_PROTECTION_TRANSLATION(f) \
  f(satp       , 0x180)

/** Debug/Trace Registers (Trigger Module Registers) **/
#ifdef CONFIG_RVSDTRIG
  #define CSRS_S_DEBUG_TRACE(f) \
    f(scontext   , 0x6A8)
#else // CONFIG_RVSDTRIG
  #define CSRS_S_DEBUG_TRACE(f)
#endif // CONFIG_RVSDTRIG

/** Supervisor State Enable Registers **/
#define CSRS_S_STATE_ENABLE(f)

/** Supervisor Custom 1 **/
#ifdef CONFIG_RV_SVINVAL
  #define CSRS_S_XIANGSHAN_SRNCTL(f) \
    f(srnctl     , 0x5c4)
#else
  #define CSRS_S_XIANGSHAN_SRNCTL(f)
#endif

#define CSRS_S_XIANGSHAN_CTRL(f) \
  CSRS_S_XIANGSHAN_SRNCTL(f)

#define CSRS_S_CUSTOM_1(f) \
  CSRS_S_XIANGSHAN_CTRL(f)

/** ALL **/
#define CSRS_S(f) \
  CSRS_S_TRAP_SETUP(f) \
  CSRS_S_CONFIGURATION(f) \
  CSRS_S_COUNTER_SETUP(f) \
  CSRS_S_TRAP_HANDLING(f) \
  CSRS_S_PROTECTION_TRANSLATION(f) \
  CSRS_S_DEBUG_TRACE(f) \
  CSRS_S_STATE_ENABLE(f) \
  CSRS_S_CUSTOM_1(f)


/* hypervisor and Virtual Supervisor CSR */
#ifdef CONFIG_RVH
  /** Hypervisor Trap Setup **/
  #define CSRS_H_TRAP_SETUP(f) \
    f(hstatus    , 0x600) f(hedeleg    , 0x602) f(hideleg    , 0x603) \
    f(hie        , 0x604) f(hcounteren , 0x606) f(hgeie      , 0x607)

  /** Hypervisor Trap Handling **/
  #define CSRS_H_TRAP_HANDLING(f) \
    f(htval      , 0x643) f(hip        , 0x644) f(hvip       , 0x645) \
    f(htinst     , 0x64A) f(hgeip      , 0xE12)

  /** Hypervisor Configuration **/
  #define CSRS_H_CONFIGURATION(f) \
    f(henvcfg    , 0x60A)

  /** Hypervisor Protection and Translation **/
  #define CSRS_H_PROTECTION_TRANSLATION(f) \
    f(hgatp      , 0x680)

  /** Debug/Trace Registers (Trigger Module Registers) **/
  #ifdef CONFIG_RVSDTRIG
    #define CSRS_H_DEBUG_TRACE(f) \
      f(hcontext   , 0x6A8)
  #else // CONFIG_RVSDTRIG
    #define CSRS_H_DEBUG_TRACE(f)
  #endif // CONFIG_RVSDTRIG

  /** Hypervisor Counter/Timer Virtualization Registers **/
  #define CSRS_H_CONUTER_TIMER_VIRTUALIZATION(f) \
    f(htimedelta , 0x605)

  /** Virtual Supervisor Registers **/
  #define CSRS_VS(f) \
    f(vsstatus   , 0x200) f(vsie       , 0x204) f(vstvec     , 0x205) \
    f(vsscratch  , 0x240) f(vsepc      , 0x241) f(vscause    , 0x242) \
    f(vstval     , 0x243) f(vsip       , 0x244) f(vsatp      , 0x280)

  /** ALL **/
  #define CSRS_H_VS(f) \
    CSRS_H_TRAP_SETUP(f) \
    CSRS_H_TRAP_HANDLING(f) \
    CSRS_H_CONFIGURATION(f) \
    CSRS_H_PROTECTION_TRANSLATION(f) \
    CSRS_H_DEBUG_TRACE(f) \
    CSRS_H_CONUTER_TIMER_VIRTUALIZATION(f) \
    CSRS_VS(f)

#else // CONFIG_RVH
  #define CSRS_H_VS(f)
#endif // CONFIG_RVH


/* Machine-level CSR */
/** Machine Information Registers **/
#define CSRS_M_INFOMATION(f) \
  f(mvendorid  , 0xF11) f(marchid    , 0xF12) f(mimpid     , 0xF13) \
  f(mhartid    , 0xF14) f(mconfigptr , 0xF15)

/** Machine Trap Setup **/
#define CSRS_M_TRAP_SETUP(f) \
  f(mstatus    , 0x300) f(misa       , 0x301) f(medeleg    , 0x302) \
  f(mideleg    , 0x303) f(mie        , 0x304) f(mtvec      , 0x305) \
  f(mcounteren , 0x306) \

/** Machine Trap Handling **/
#ifdef CONFIG_RVH
  #define CSRS_M_GUEST_TRAP_HANDLING(f) \
    f(mtinst     , 0x34A) f(mtval2     , 0x34B)
#else // CONFIG_RVH
  #define CSRS_M_GUEST_TRAP_HANDLING(f)
#endif // CONFIG_RVH

#define CSRS_M_TRAP_HANDLING(f) \
  f(mscratch   , 0x340) f(mepc       , 0x341) f(mcause     , 0x342) \
  f(mtval      , 0x343) f(mip        , 0x344) \
  CSRS_M_GUEST_TRAP_HANDLING(f)

/** Machine Configuration **/
#define CSRS_M_CONFIGURATION(f) \
  f(menvcfg    , 0x30A)

/** Machine Memory Protection (PMP) **/
#ifdef CONFIG_RV_PMP_ENTRY_0
  #define CSRS_M_MEMORY_PROTECTION(f)
#endif // CONFIG_RV_PMP_ENTRY_0
#ifdef CONFIG_RV_PMP_ENTRY_16
  #define CSRS_M_MEMORY_PROTECTION(f) \
    f(pmpcfg0    , 0x3A0) f(pmpcfg2    , 0x3A2) \
    f(pmpaddr0   , 0x3B0) f(pmpaddr1   , 0x3B1) f(pmpaddr2   , 0x3B2) f(pmpaddr3   , 0x3B3) \
    f(pmpaddr4   , 0x3B4) f(pmpaddr5   , 0x3B5) f(pmpaddr6   , 0x3B6) f(pmpaddr7   , 0x3B7) \
    f(pmpaddr8   , 0x3B8) f(pmpaddr9   , 0x3B9) f(pmpaddr10  , 0x3BA) f(pmpaddr11  , 0x3BB) \
    f(pmpaddr12  , 0x3BC) f(pmpaddr13  , 0x3BD) f(pmpaddr14  , 0x3BE) f(pmpaddr15  , 0x3BF)
#endif // CONFIG_RV_PMP_ENTRY_16
#ifdef CONFIG_RV_PMP_ENTRY_64
  #define CSRS_M_MEMORY_PROTECTION(f) \
    f(pmpcfg0    , 0x3A0) f(pmpcfg2    , 0x3A2) f(pmpcfg4    , 0x3A4) f(pmpcfg6    , 0x3A6)\
    f(pmpcfg8    , 0x3A8) f(pmpcfg10   , 0x3AA) f(pmpcfg12   , 0x3AC) f(pmpcfg14   , 0x3AE)\
    f(pmpaddr0   , 0x3B0) f(pmpaddr1   , 0x3B1) f(pmpaddr2   , 0x3B2) f(pmpaddr3   , 0x3B3) \
    f(pmpaddr4   , 0x3B4) f(pmpaddr5   , 0x3B5) f(pmpaddr6   , 0x3B6) f(pmpaddr7   , 0x3B7) \
    f(pmpaddr8   , 0x3B8) f(pmpaddr9   , 0x3B9) f(pmpaddr10  , 0x3BA) f(pmpaddr11  , 0x3BB) \
    f(pmpaddr12  , 0x3BC) f(pmpaddr13  , 0x3BD) f(pmpaddr14  , 0x3BE) f(pmpaddr15  , 0x3BF) \
    f(pmpaddr16  , 0x3C0) f(pmpaddr17  , 0x3C1) f(pmpaddr18  , 0x3C2) f(pmpaddr19  , 0x3C3) \
    f(pmpaddr20  , 0x3C4) f(pmpaddr21  , 0x3C5) f(pmpaddr22  , 0x3C6) f(pmpaddr23  , 0x3C7) \
    f(pmpaddr24  , 0x3C8) f(pmpaddr25  , 0x3C9) f(pmpaddr26  , 0x3CA) f(pmpaddr27  , 0x3CB) \
    f(pmpaddr28  , 0x3CC) f(pmpaddr29  , 0x3CD) f(pmpaddr30  , 0x3CE) f(pmpaddr31  , 0x3CF) \
    f(pmpaddr32  , 0x3D0) f(pmpaddr33  , 0x3D1) f(pmpaddr34  , 0x3D2) f(pmpaddr35  , 0x3D3) \
    f(pmpaddr36  , 0x3D4) f(pmpaddr37  , 0x3D5) f(pmpaddr38  , 0x3D6) f(pmpaddr39  , 0x3D7) \
    f(pmpaddr40  , 0x3D8) f(pmpaddr41  , 0x3D9) f(pmpaddr42  , 0x3DA) f(pmpaddr43  , 0x3DB) \
    f(pmpaddr44  , 0x3DC) f(pmpaddr45  , 0x3DD) f(pmpaddr46  , 0x3DE) f(pmpaddr47  , 0x3DF) \
    f(pmpaddr48  , 0x3E0) f(pmpaddr49  , 0x3E1) f(pmpaddr50  , 0x3E2) f(pmpaddr51  , 0x3E3) \
    f(pmpaddr52  , 0x3E4) f(pmpaddr53  , 0x3E5) f(pmpaddr54  , 0x3E6) f(pmpaddr55  , 0x3E7) \
    f(pmpaddr56  , 0x3E8) f(pmpaddr57  , 0x3E9) f(pmpaddr58  , 0x3EA) f(pmpaddr59  , 0x3EB) \
    f(pmpaddr60  , 0x3EC) f(pmpaddr61  , 0x3ED) f(pmpaddr62  , 0x3EE) f(pmpaddr63  , 0x3EF)
#endif // CONFIG_RV_PMP_ENTRY_64

/** Machine State Enable Registers **/
#define CSRS_M_STATE_ENABLE(f)

/** Machine Non-Maskable Interrupt Handling **/
#define CSRS_M_NON_MASKABLE_INTERRUPT_HANDLING(f)

/** Machine Counter/Timers **/
#define CSRS_M_CNTR(f) \
  f(mcycle     , 0xB00) f(minstret   , 0xB02)

#define CSRS_M_HPMCOUNTER(f) \
  f(mhpmcounter3   , 0xB03) \
  f(mhpmcounter4   , 0xB04) f(mhpmcounter5   , 0xB05) f(mhpmcounter6   , 0xB06) f(mhpmcounter7   , 0xB07) \
  f(mhpmcounter8   , 0xB08) f(mhpmcounter9   , 0xB09) f(mhpmcounter10  , 0xB0A) f(mhpmcounter11  , 0xB0B) \
  f(mhpmcounter12  , 0xB0C) f(mhpmcounter13  , 0xB0D) f(mhpmcounter14  , 0xB0E) f(mhpmcounter15  , 0xB0F) \
  f(mhpmcounter16  , 0xB10) f(mhpmcounter17  , 0xB11) f(mhpmcounter18  , 0xB12) f(mhpmcounter19  , 0xB13) \
  f(mhpmcounter20  , 0xB14) f(mhpmcounter21  , 0xB15) f(mhpmcounter22  , 0xB16) f(mhpmcounter23  , 0xB17) \
  f(mhpmcounter24  , 0xB18) f(mhpmcounter25  , 0xB19) f(mhpmcounter26  , 0xB1A) f(mhpmcounter27  , 0xB1B) \
  f(mhpmcounter28  , 0xB1C) f(mhpmcounter29  , 0xB1D) f(mhpmcounter30  , 0xB1E) f(mhpmcounter31  , 0xB1F)

#define CSRS_M_COUNTER_TIMERS(f)\
  CSRS_M_CNTR(f) \
  CSRS_M_HPMCOUNTER(f)

/** Machine Counter Setup **/
#define CSRS_M_HPMEVENT(f) \
  f(mhpmevent3     , 0x323) \
  f(mhpmevent4     , 0x324) f(mhpmevent5     , 0x325) f(mhpmevent6     , 0x326) f(mhpmevent7     , 0x327) \
  f(mhpmevent8     , 0x328) f(mhpmevent9     , 0x329) f(mhpmevent10    , 0x32A) f(mhpmevent11    , 0x32B) \
  f(mhpmevent12    , 0x32C) f(mhpmevent13    , 0x32D) f(mhpmevent14    , 0x32E) f(mhpmevent15    , 0x32F) \
  f(mhpmevent16    , 0x330) f(mhpmevent17    , 0x331) f(mhpmevent18    , 0x332) f(mhpmevent19    , 0x333) \
  f(mhpmevent20    , 0x334) f(mhpmevent21    , 0x335) f(mhpmevent22    , 0x336) f(mhpmevent23    , 0x337) \
  f(mhpmevent24    , 0x338) f(mhpmevent25    , 0x339) f(mhpmevent26    , 0x33A) f(mhpmevent27    , 0x33B) \
  f(mhpmevent28    , 0x33C) f(mhpmevent29    , 0x33D) f(mhpmevent30    , 0x33E) f(mhpmeven31     , 0x33F)

#ifdef CONFIG_RV_CSR_MCOUNTINHIBIT
  #define CSRS_M_MCOUNTINHIBIT(f) \
  f(mcountinhibit  , 0x320)
#else // CONFIG_RV_CSR_MCOUNTINHIBIT
  #define CSRS_M_MCOUNTINHIBIT(f)
#endif // CONFIG_RV_CSR_MCOUNTINHIBIT

#define CSRS_M_COUNTER_SETUP(f) \
  CSRS_M_MCOUNTINHIBIT(f) \
  CSRS_M_HPMEVENT(f)
  
/** Debug/Trace Registers (Trigger Module Registers) **/
#ifdef CONFIG_RVSDTRIG
  #define CSRS_M_DEBUG_TRACE(f) \
    f(tselect    , 0x7A0) \
    f(tdata1     , 0x7A1) f(tdata2     , 0x7A2) f(tdata3     , 0x7A3) \
    f(tinfo      , 0x7A4) f(tcontrol   , 0x7A5) \
    f(mcontext   , 0x7A8)
#else // CONFIG_RVSDTRIG
  #define CSRS_M_DEBUG_TRACE(f)
#endif // CONFIG_RVSDTRIG

/** Debug Mode Registers (Core Debug Registers) **/
#ifdef CONFIG_RVSDEXT
  #define CSRS_DEBUG_MODE(f) \
    f(dcsr       , 0x7b0) f(dpc        , 0x7b1) \
    f(dscratch0  , 0x7b2) f(dscratch1  , 0x7b3)
#else // CONFIG_RVSDEXT
  #define CSRS_DEBUG_MODE(f)
#endif // CONFIG_RVSDEXT

/** ALL **/
#define CSRS_M(f) \
  CSRS_M_INFOMATION(f) \
  CSRS_M_TRAP_SETUP(f) \
  CSRS_M_TRAP_HANDLING(f) \
  CSRS_M_CONFIGURATION(f) \
  CSRS_M_MEMORY_PROTECTION(f) \
  CSRS_M_STATE_ENABLE(f) \
  CSRS_M_NON_MASKABLE_INTERRUPT_HANDLING(f) \
  CSRS_M_COUNTER_TIMERS(f) \
  CSRS_M_COUNTER_SETUP(f) \
  CSRS_M_DEBUG_TRACE(f) \
  CSRS_DEBUG_MODE(f)


/* ALL CSRs */
#define CSRS(f) \
  CSRS_UNPRIV(f) \
  CSRS_S(f) \
  CSRS_H_VS(f) \
  CSRS_M(f)


/**
 * Sturcture of CSRs
*/

/* Macros */

#define CSR_STRUCT_START(name) \
  typedef union { \
    struct {

#define CSR_STRUCT_END(name) \
    }; \
    word_t val; \
  } concat(name, _t);

#define CSR_STRUCT_DUMMY(name, addr) \
  typedef union { \
    struct {}; \
    word_t val; \
  } concat(name, _t);

#define CSR_STRUCT_DUMMY_LIST(list) \
  MAP(list, CSR_STRUCT_DUMMY)

/* Machine-Level CSR */

CSR_STRUCT_START(misa)
  uint64_t extensions: 26;
  uint64_t pad       : 36;
  uint64_t mxl       :  2;
CSR_STRUCT_END(misa)

CSR_STRUCT_START(mvendorid)
CSR_STRUCT_END(mvendorid)

CSR_STRUCT_START(marchid)
CSR_STRUCT_END(marchid)

CSR_STRUCT_START(mimpid)
CSR_STRUCT_END(mimpid)

CSR_STRUCT_START(mhartid)
CSR_STRUCT_END(mhartid)

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
  uint64_t vs  : 2;
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

typedef enum ExtContextStatus {
  EXT_CONTEXT_DISABLED = 0,
  EXT_CONTEXT_INITIAL,
  EXT_CONTEXT_CLEAN,
  EXT_CONTEXT_DIRTY,
} ExtContextStatus;

CSR_STRUCT_START(mtvec)
CSR_STRUCT_END(mtvec)

CSR_STRUCT_START(medeleg)
CSR_STRUCT_END(medeleg)

CSR_STRUCT_START(mideleg)
CSR_STRUCT_END(mideleg)

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

CSR_STRUCT_START(mcycle)
CSR_STRUCT_END(mcycle)

CSR_STRUCT_START(minstret)
CSR_STRUCT_END(minstret)

CSR_STRUCT_DUMMY_LIST(CSRS_M_HPMCOUNTER)
CSR_STRUCT_DUMMY_LIST(CSRS_M_HPMEVENT)

CSR_STRUCT_START(mcounteren)
CSR_STRUCT_END(mcounteren)

#ifdef CONFIG_RV_CSR_MCOUNTINHIBIT
CSR_STRUCT_START(mcountinhibit)
CSR_STRUCT_END(mcountinhibit)
#endif // CONFIG_RV_CSR_MCOUNTINHIBIT

CSR_STRUCT_START(mscratch)
CSR_STRUCT_END(mscratch)

CSR_STRUCT_START(mepc)
CSR_STRUCT_END(mepc)

CSR_STRUCT_START(mcause)
  uint64_t code:63;
  uint64_t intr: 1;
CSR_STRUCT_END(mcause)

CSR_STRUCT_START(mtval)
CSR_STRUCT_END(mtval)

CSR_STRUCT_START(mconfigptr)
CSR_STRUCT_END(mconfigptr)

CSR_STRUCT_START(menvcfg)
CSR_STRUCT_END(menvcfg)

/** "H" Hypervisor Extension CSRs **/

#ifdef CONFIG_RVH
CSR_STRUCT_START(mtval2)
CSR_STRUCT_END(mtval2)

CSR_STRUCT_START(mtinst)
CSR_STRUCT_END(mtinst)
#endif

/** Physical Memory Protection CSRs */

CSR_STRUCT_DUMMY_LIST(CSRS_M_MEMORY_PROTECTION)

/** Debug Mode Registers (Core Debug Registers) **/

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

/** Debug/Trace Registers (Trigger Module Registers) **/

#ifdef CONFIG_RVSDTRIG
CSR_STRUCT_START(scontext)  // 0x5a8
CSR_STRUCT_END(scontext)

#ifdef CONFIG_RVH
  CSR_STRUCT_START(hcontext)  //  0x6a8
  CSR_STRUCT_END(hcontext)
#endif // CONFIG_RVH

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

/* Supervisor-level CSR */

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

CSR_STRUCT_START(scounteren)
CSR_STRUCT_END(scounteren)

CSR_STRUCT_START(sscratch)
CSR_STRUCT_END(sscratch)

CSR_STRUCT_START(sepc)
CSR_STRUCT_END(sepc)

CSR_STRUCT_START(scause)
  uint64_t code:63;
  uint64_t intr: 1;
CSR_STRUCT_END(scause)

CSR_STRUCT_START(stval)
CSR_STRUCT_END(stval)

CSR_STRUCT_START(senvcfg)
CSR_STRUCT_END(senvcfg)

CSR_STRUCT_START(satp)
  uint64_t ppn :44;
  uint64_t asid:16;
  uint64_t mode: 4;
CSR_STRUCT_END(satp)

/** Supervisor Custom CSRs **/

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


/* hypervisor and Virtual Supervisor CSR */

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

CSR_STRUCT_START(hvip)
  uint64_t pad0  : 2;
  uint64_t vssip : 1;
  uint64_t pad1  : 3;
  uint64_t vstip : 1;
  uint64_t pad2  : 3;
  uint64_t vseip : 1;
CSR_STRUCT_END(hvip)

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

CSR_STRUCT_START(hgeip)
CSR_STRUCT_END(hgeip)

CSR_STRUCT_START(hgeie)
CSR_STRUCT_END(hgeie)

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

CSR_STRUCT_START(hcounteren)
CSR_STRUCT_END(hcounteren)

CSR_STRUCT_START(htimedelta)
CSR_STRUCT_END(htimedelta)

CSR_STRUCT_START(htval)
CSR_STRUCT_END(htval)

CSR_STRUCT_START(htinst)
CSR_STRUCT_END(htinst)

CSR_STRUCT_START(hgatp)
  uint64_t ppn    : 44;
  uint64_t vmid   : 14;
  uint64_t pad0   : 2;
  uint64_t mode   : 4;
CSR_STRUCT_END(hgatp)

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

/* Unprivileged CSR */

/** Unprivileged Floating-Point CSRs **/

#ifndef CONFIG_FPU_NONE
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

#endif // CONFIG_FPU_NONE

/** Unprivileged Vector CSRs **/

#ifdef CONFIG_RVV

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

#ifdef CONFIG_RV_Zicntr
CSR_STRUCT_START(cycle)
CSR_STRUCT_END(cycle)

#ifdef CONFIG_RV_CSR_TIME
CSR_STRUCT_START(csr_time)
CSR_STRUCT_END(csr_time)
#endif // CONFIG_RV_CSR_TIME

CSR_STRUCT_START(instret)
CSR_STRUCT_END(instret)
#endif // CONFIG_RV_Zicntr

#ifdef CONFIG_RV_Zihpm
CSR_STRUCT_DUMMY_LIST(CSRS_UNPRIV_HPMCOUNTER)
#endif // CONFIG_RV_Zihpm


/**
 * Declare pointers to CSRs
*/

#define CSRS_DECL(name, addr) extern concat(name, _t)* const name;
MAP(CSRS, CSRS_DECL)


/**
 * Useful Defines
*/

/** Counters/Timers **/
#define CSR_HPMCOUNTER_BASE     0xC03
#define CSR_HPMCOUNTER_NUM      29
#define CSR_MHPMCOUNTER_BASE    0xB03
#define CSR_MHPMCOUNTER_NUM     29
#define CSR_MHPMEVENT_BASE      0x323
#define CSR_MHPMEVENT_NUM       29

/** Machine Memory Protection (PMP) **/
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

#define CSR_PMPCFG_BASE     0x3a0
#define CSR_PMPADDR_BASE    0x3b0
#define CSR_PMPCFG_MAX_NUM  16
#define CSR_PMPADDR_MAX_NUM 64

/** Vector **/
#define IDXVSTART 0x008
#define IDXVXSAT  0x009
#define IDXVXRM   0x00a
#define IDXVL     0xc20
#define IDXVTYPE  0xc21
#define IDXVLENB  0xc22

/** CSR satp **/
#define SATP_MODE_BARE 0
#define SATP_MODE_Sv39 8
#define SATP_ASID_LEN 16 // max is 16
#define SATP_PADDR_LEN (CONFIG_PADDRBITS-12) // max is 44
#define SATP_ASID_MAX_LEN 16
#define SATP_PADDR_MAX_LEN 44

#define SATP_MODE_MASK (8UL << (SATP_ASID_MAX_LEN + SATP_PADDR_MAX_LEN))
#define SATP_ASID_MASK (((1L << SATP_ASID_LEN)-1) << SATP_PADDR_MAX_LEN)
#define SATP_PADDR_MASK ((1L << SATP_PADDR_LEN)-1)

#define SATP_MASK (SATP_MODE_MASK | SATP_ASID_MASK | SATP_PADDR_MASK)
#define MASKED_SATP(x) (SATP_MASK & x)

/** CSR hgatp **/
#ifdef CONFIG_RVH
#define HGATP_MODE_BARE   0
#define HGATP_MODE_Sv39x4 8
#define HGATP_VMID_LEN 14 // max is 14
#define HGATP_PADDR_LEN 44 // max is 44
#define HGATP_VMID_MAX_LEN 16
#define HGATP_PADDR_MAX_LEN 44

#define HGATP_MODE_MASK (8UL << (HGATP_VMID_MAX_LEN + HGATP_PADDR_MAX_LEN))
#define HGATP_VMID_MASK (((1L << HGATP_VMID_LEN)-1) << HGATP_PADDR_MAX_LEN)
#define HGATP_PADDR_MASK ((1L << HGATP_PADDR_MAX_LEN)-1)

#define HGATP_MASK (HGATP_MODE_MASK | HGATP_VMID_MASK | HGATP_PADDR_MASK)
#endif // CONFIG_RVH

#ifdef CONFIG_RVH
#define HGATP_Bare_GPADDR_LEN CONFIG_PADDRBITS
#define HGATP_Sv39x4_GPADDR_LEN 41
#define VSATP_PPN_HGATP_BARE_MASK BITMASK(HGATP_Bare_GPADDR_LEN - PAGE_SHIFT)
#define VSATP_PPN_HGATP_Sv39x4_MASK BITMASK(HGATP_Sv39x4_GPADDR_LEN - PAGE_SHIFT)
#endif // CONFIG_RVH

/** RVH **/
#ifdef CONFIG_RVH
  extern bool v; // virtualization mode
  #define vsatp_mode ((hstatus->vsxl == 1)? vsatp->_32.mode : vsatp->_64.mode)
  #define vsatp_asid ((hstatus->vsxl == 1)? vsatp->_32.asid : vsatp->_64.asid)
  #define vsatp_ppn  ((hstatus->vsxl == 1)? vsatp->_32.ppn  : vsatp->_64.ppn)
  #define _vsstatus_  ((hstatus->vsxl == 1)? vsstatus->_32  : vsstatus->_64)
#endif // CONFIG_RVH

/** SSTATUS **/
// All valid fields defined by RISC-V spec and not affected by extensions
// This mask is used to get the value of sstatus from mstatus
// SD, UXL, MXR, SUM, XS, FS, VS, SPP, UBE, SPIE, SIE
#define SSTATUS_RMASK 0x80000003000de762UL


/**
 * Function declaration
*/

/** General **/
void csr_prepare();

word_t gen_status_sd(word_t status);

word_t csrid_read(uint32_t csrid);

/** PMP **/
uint8_t pmpcfg_from_index(int idx);
word_t pmpaddr_from_index(int idx);
word_t pmp_tor_mask();

#endif // __CSR_H__
