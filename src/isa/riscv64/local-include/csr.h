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

#define FUNCT3_CSRRW  1
#define FUNCT3_CSRRS  2
#define FUNCT3_CSRRC  3
#define FUNCT3_CSRRWI 5
#define FUNCT3_CSRRSI 6
#define FUNCT3_CSRRCI 7

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

#ifdef CONFIG_RV_ZICNTR
  #define CSRS_UNPRIV_CNTR(f) \
    f(cycle      , 0xC00) \
    CSRS_UNPRIV_TIME(f) \
    f(instret    , 0xC02)
    // There is `time_t` type in the C programming language.
    // So We have to use another name for CSR time.
#else // CONFIG_RV_ZICNTR
  #define CSRS_UNPRIV_CNTR(f)
#endif // CONFIG_RV_ZICNTR

#ifdef CONFIG_RV_ZIHPM
  #define CSRS_UNPRIV_HPMCOUNTER(f) \
    f(hpmcounter3    , 0xC03) \
    f(hpmcounter4    , 0xC04) f(hpmcounter5    , 0xC05) f(hpmcounter6    , 0xC06) f(hpmcounter7    , 0xC07) \
    f(hpmcounter8    , 0xC08) f(hpmcounter9    , 0xC09) f(hpmcounter10   , 0xC0A) f(hpmcounter11   , 0xC0B) \
    f(hpmcounter12   , 0xC0C) f(hpmcounter13   , 0xC0D) f(hpmcounter14   , 0xC0E) f(hpmcounter15   , 0xC0F) \
    f(hpmcounter16   , 0xC10) f(hpmcounter17   , 0xC11) f(hpmcounter18   , 0xC12) f(hpmcounter19   , 0xC13) \
    f(hpmcounter20   , 0xC14) f(hpmcounter21   , 0xC15) f(hpmcounter22   , 0xC16) f(hpmcounter23   , 0xC17) \
    f(hpmcounter24   , 0xC18) f(hpmcounter25   , 0xC19) f(hpmcounter26   , 0xC1A) f(hpmcounter27   , 0xC1B) \
    f(hpmcounter28   , 0xC1C) f(hpmcounter29   , 0xC1D) f(hpmcounter30   , 0xC1E) f(hpmcounter31   , 0xC1F)
#else // CONFIG_RV_ZIHPM
  #define CSRS_UNPRIV_HPMCOUNTER(f)
#endif // CONFIG_RV_ZIHPM

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
#ifdef CONFIG_RV_SDTRIG
  #define CSRS_S_DEBUG_TRACE(f) \
    f(scontext   , 0x6A8)
#else // CONFIG_RV_SDTRIG
  #define CSRS_S_DEBUG_TRACE(f)
#endif // CONFIG_RV_SDTRIG

/** Supervisor State Enable Registers **/
#ifdef CONFIG_RV_SMSTATEEN
  #define CSRS_S_STATE_ENABLE(f) \
    f(sstateen0 , 0x10C) 
#else
  #define CSRS_S_STATE_ENABLE(f)
#endif // CONFIG_RV_SMSTATEEN

/** Supervisor Counter Overflow Register **/
#ifdef CONFIG_RV_SSCOFPMF
  #define CSRS_S_SCOFPMF(f) \
    f(scountovf, 0xDA0)
#else
  #define CSRS_S_SCOFPMF(f)
#endif // CONFIG_RV_SSCOFPMF


/** Supervisor Custom 1 **/
#ifdef CONFIG_RV_SVINVAL
  #define CSRS_S_XIANGSHAN_SRNCTL(f) \
    f(srnctl     , 0x5c4)
#else
  #define CSRS_S_XIANGSHAN_SRNCTL(f)
#endif

#define CSRS_S_XIANGSHAN_CTRL(f) \
  CSRS_S_XIANGSHAN_SRNCTL(f) \
  f(sbpctl  ,   0x5c0) \
  f(spfctl  ,   0x5c1) \
  f(slvpredctl, 0x5c2) \
  f(smblockctl, 0x5c3) \
  f(sfetchctl,  0x9e0)    

#define CSRS_S_CUSTOM_1(f) \
  CSRS_S_XIANGSHAN_CTRL(f)

/** Supervisor Timer Register **/
#ifdef CONFIG_RV_SSTC
  #define CSRS_S_SSTC(f) \
    f(stimecmp, 0x14D)
#else
  #define CSRS_S_SSTC(f)
#endif


/** Supervisor Advanced Interrupt Architecture Registers **/
#ifdef CONFIG_RV_IMSIC
  #define CSRS_S_AIA(f) \
    f(siselect , 0x150) f(sireg   , 0x151) \
    f(stopei   , 0x15C) f(stopi   , 0xDB0)
#else
  #define CSRS_S_AIA(f)
#endif // CONFIG_RV_IMSIC

/** ALL **/
#define CSRS_S(f) \
  CSRS_S_TRAP_SETUP(f) \
  CSRS_S_CONFIGURATION(f) \
  CSRS_S_COUNTER_SETUP(f) \
  CSRS_S_TRAP_HANDLING(f) \
  CSRS_S_PROTECTION_TRANSLATION(f) \
  CSRS_S_DEBUG_TRACE(f) \
  CSRS_S_STATE_ENABLE(f) \
  CSRS_S_SCOFPMF(f) \
  CSRS_S_AIA(f) \
  CSRS_S_SSTC(f) \
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
  #ifdef CONFIG_RV_SDTRIG
    #define CSRS_H_DEBUG_TRACE(f) \
      f(hcontext   , 0x6A8)
  #else // CONFIG_RV_SDTRIG
    #define CSRS_H_DEBUG_TRACE(f)
  #endif // CONFIG_RV_SDTRIG

  /** Hypervisor Counter/Timer Virtualization Registers **/
  #define CSRS_H_CONUTER_TIMER_VIRTUALIZATION(f) \
    f(htimedelta , 0x605)

  /** Hypervisor State Enable Registers **/
  #ifdef CONFIG_RV_SMSTATEEN
    #define CSRS_H_STATE_ENABLE(f) \
      f(hstateen0 , 0x60C) 
  #else
    #define CSRS_H_STATE_ENABLE(f)
  #endif // CONFIG_RV_SMSTATEEN

  /** Virtual Supervisor Registers **/
  #define CSRS_VS(f) \
    f(vsstatus   , 0x200) f(vsie       , 0x204) f(vstvec     , 0x205) \
    f(vsscratch  , 0x240) f(vsepc      , 0x241) f(vscause    , 0x242) \
    f(vstval     , 0x243) f(vsip       , 0x244) f(vsatp      , 0x280)

  /** Hypervisor and VS AIA Registers **/
  #ifdef CONFIG_RV_IMSIC
    #define CSRS_H_VS_AIA(f) \
      f(vsiselect  , 0x250) f(vsireg     , 0x251) \
      f(vstopei    , 0x25C) f(hvien      , 0x608) \
      f(hvictl     , 0x609) f(hviprio1   , 0x646) \
      f(hviprio2   , 0x647) f(vstopi     , 0xEB0)
  #else
    #define CSRS_H_VS_AIA(f)
  #endif // CONFIG_RV_IMSIC

  #ifdef CONFIG_RV_SSTC
    #define CSRS_VS_SSTC(f) \
      f(vstimecmp , 0x24D)
  #else
    #define CSRS_VS_SSTC(f)
  #endif

  /** ALL **/
  #define CSRS_H_VS(f) \
    CSRS_H_TRAP_SETUP(f) \
    CSRS_H_TRAP_HANDLING(f) \
    CSRS_H_CONFIGURATION(f) \
    CSRS_H_PROTECTION_TRANSLATION(f) \
    CSRS_H_DEBUG_TRACE(f) \
    CSRS_H_CONUTER_TIMER_VIRTUALIZATION(f) \
    CSRS_H_STATE_ENABLE(f) \
    CSRS_H_VS_AIA(f) \
    CSRS_VS_SSTC(f) \
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
  f(menvcfg    , 0x30A) f(mseccfg    , 0x747)

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
#ifdef CONFIG_RV_SMSTATEEN
  #define CSRS_M_STATE_ENABLE(f) \
    f(mstateen0, 0x30C)
#else
  #define CSRS_M_STATE_ENABLE(f)
#endif // CONFIG_RV_SMSTATEEN

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
#ifdef CONFIG_RV_SDTRIG
  #define CSRS_M_DEBUG_TRACE(f) \
    f(tselect    , 0x7A0) \
    f(tdata1     , 0x7A1) f(tdata2     , 0x7A2) \
    f(tinfo      , 0x7A4) \
    IFDEF(CONFIG_SDTRIG_EXTRA, f(tdata3  , 0x7A3)) \
    IFDEF(CONFIG_SDTRIG_EXTRA, f(mcontext, 0x7A8))
#else // CONFIG_RV_SDTRIG
  #define CSRS_M_DEBUG_TRACE(f)
#endif // CONFIG_RV_SDTRIG

/** Debug Mode Registers (Core Debug Registers) **/
#ifdef CONFIG_RV_SDEXT
  #define CSRS_DEBUG_MODE(f) \
    f(dcsr       , 0x7b0) f(dpc        , 0x7b1) \
    f(dscratch0  , 0x7b2) f(dscratch1  , 0x7b3)
#else // CONFIG_RV_SDEXT
  #define CSRS_DEBUG_MODE(f)
#endif // CONFIG_RV_SDEXT

/** Machine AIA Registers **/
#ifdef CONFIG_RV_IMSIC
  #define CSRS_M_AIA(f) \
  f(mvien      , 0x308) f(mvip       , 0x309) \
  f(miselect   , 0x350) f(mireg      , 0x351) \
  f(mtopei     , 0x35C) f(mtopi      , 0xFB0)
#else
  #define CSRS_M_AIA(f)
#endif // CONFIG_RV_IMSIC

/**  Machine Non-Maskable Interrupt Handling **/
#ifdef CONFIG_RV_SMRNMI
  #define CSRS_M_SMRNMI(f) \
  f(mnepc      , 0x741) f(mncause    , 0x742) \
  f(mnstatus   , 0x744) f(mnscratch  , 0x740)
#else
  #define CSRS_M_SMRNMI(f)
#endif //CONFIG_RV_SMRNMI

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
  CSRS_M_AIA(f) \
  CSRS_M_SMRNMI(f) \
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
  uint64_t pad0: 1; // [0]
  uint64_t sie : 1; // [1]
  uint64_t pad1: 1; // [2]
  uint64_t mie : 1; // [3]
  uint64_t pad2: 1; // [4]
  uint64_t spie: 1; // [5]
  uint64_t ube : 1; // [6]
  uint64_t mpie: 1; // [7]
  uint64_t spp : 1; // [8]
  uint64_t vs  : 2; // [10:9]
  uint64_t mpp : 2; // [12:11]
  uint64_t fs  : 2; // [14:13]
  uint64_t xs  : 2; // [16:15]
  uint64_t mprv: 1; // [17]
  uint64_t sum : 1; // [18]
  uint64_t mxr : 1; // [19]
  uint64_t tvm : 1; // [20]
  uint64_t tw  : 1; // [21]
  uint64_t tsr : 1; // [22]
  uint64_t pad3: 1; // [23]
  uint64_t sdt : 1; // [24]
  uint64_t pad4: 7; // [31:25]
  uint64_t uxl : 2; // [33:32]
  uint64_t sxl : 2; // [35:34]
  uint64_t sbe : 1; // [36]
  uint64_t mbe : 1; // [37]
#ifdef CONFIG_RVH
  uint64_t gva : 1; // [38]
  uint64_t mpv : 1; // [39]
#else
  uint64_t pad5: 2; // [39:38]
#endif
  uint64_t pad6: 2; // [41:40]
  uint64_t mdt : 1; // [42]
  uint64_t pad7:20; // [62:43]
  uint64_t sd  : 1; // [63]
CSR_STRUCT_END(mstatus)

typedef enum ExtContextStatus {
  EXT_CONTEXT_DISABLED = 0,
  EXT_CONTEXT_INITIAL,
  EXT_CONTEXT_CLEAN,
  EXT_CONTEXT_DIRTY,
} ExtContextStatus;

CSR_STRUCT_START(tvec)
  uint64_t mode  : 2;
  uint64_t base  :62;
CSR_STRUCT_END(tvec)

typedef tvec_t mtvec_t;

CSR_STRUCT_START(medeleg)
CSR_STRUCT_END(medeleg)

CSR_STRUCT_START(mideleg)
  uint64_t usi  : 1;
  uint64_t ssi  : 1;
  uint64_t vssi : 1;
  uint64_t msi  : 1;
  uint64_t uti  : 1;
  uint64_t sti  : 1;
  uint64_t vsti : 1;
  uint64_t mti  : 1;
  uint64_t uei  : 1;
  uint64_t sei  : 1;
  uint64_t vsei : 1;
  uint64_t mei  : 1;
  uint64_t sgei : 1;
#ifdef CONFIG_RV_SSCOFPMF
  uint64_t lcofi : 1;
#endif
CSR_STRUCT_END(mideleg)

CSR_STRUCT_START(mip)
  uint64_t usip  : 1;  // [0]
  uint64_t ssip  : 1;  // [1]
  uint64_t vssip : 1;  // [2]
  uint64_t msip  : 1;  // [3]
  uint64_t utip  : 1;  // [4]
  uint64_t stip  : 1;  // [5]
  uint64_t vstip : 1;  // [6]
  uint64_t mtip  : 1;  // [7]
  uint64_t ueip  : 1;  // [8]
  uint64_t seip  : 1;  // [9]
  uint64_t vseip : 1;  // [10]
  uint64_t meip  : 1;  // [11]
  uint64_t sgeip : 1;  // [12]
  uint64_t lcofip: 1;  // [13]
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
#ifdef CONFIG_RV_SSCOFPMF
  uint64_t lcofie : 1;
#endif
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
  uint64_t fiom   : 1; // [0]
  uint64_t pad0   : 3; // [3:1]
  uint64_t cbie   : 2; // [5:4]
  uint64_t cbcfe  : 1; // [6]
  uint64_t cbze   : 1; // [7]
  uint64_t pad1   : 24;// [31:8]
  uint64_t pmm    : 2; // [33:32]
  uint64_t pad3   : 25;// [58:34]
  uint64_t dte    : 1; // [59]
  uint64_t cde    : 1; // [60]
  uint64_t adue   : 1; // [61]
  uint64_t pbmte  : 1; // [62]
  uint64_t stce   : 1; // [63]
CSR_STRUCT_END(menvcfg)

CSR_STRUCT_START(mseccfg)
  uint64_t mml   : 1; // [0]
  uint64_t mmwp  : 1; // [1]
  uint64_t rlb   : 1; // [2]
  uint64_t pad0  : 5; // [7:3]
  uint64_t useed : 1; // [8]
  uint64_t sseed : 1; // [9]
  uint64_t mlpe  : 1; // [10]
  uint64_t pad1  :53; // [63:11]
CSR_STRUCT_END(mseccfg)

#ifdef CONFIG_RV_SMSTATEEN
  CSR_STRUCT_START(mstateen0)
  uint64_t c      : 1; // [0]
  uint64_t fcsr   : 1; // [1]
  uint64_t jvt    : 1; // [2]
  uint64_t pad0   :53; // [55:3]
  uint64_t p1p13  : 1; // [56]
  uint64_t context: 1; // [57]
  uint64_t imsic  : 1; // [58]
  uint64_t aia    : 1; // [59]
  uint64_t csrind : 1; // [60]
  uint64_t pad2   : 1; // [61]
  uint64_t envcfg : 1; // [62]
  uint64_t se0    : 1; // [63]
  CSR_STRUCT_END(mstateen0)

  CSR_STRUCT_START(sstateen0)
  uint64_t c      : 1; // [0]
  uint64_t fcsr   : 1; // [1]
  uint64_t jvt    : 1; // [2]
  uint64_t pad0   :29; // [31:3]
  CSR_STRUCT_END(sstateen0)
  
#ifdef CONFIG_RVH
  CSR_STRUCT_START(hstateen0)
  uint64_t c      : 1; // [0]
  uint64_t fcsr   : 1; // [1]
  uint64_t jvt    : 1; // [2]
  uint64_t pad0   :53; // [55:3]
  uint64_t pad1   : 1; // [56]
  uint64_t context: 1; // [57]
  uint64_t imsic  : 1; // [58]
  uint64_t aia    : 1; // [59]
  uint64_t csrind : 1; // [60]
  uint64_t pad2   : 1; // [61]
  uint64_t envcfg : 1; // [62]
  uint64_t se0    : 1; // [63]
  CSR_STRUCT_END(hstateen0)
#endif
#endif

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

#ifdef CONFIG_RV_SDEXT
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
#endif // CONFIG_RV_SDEXT

/** Debug/Trace Registers (Trigger Module Registers) **/

#ifdef CONFIG_RV_SDTRIG
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

CSR_STRUCT_START(mcontext)  // 0x7a8
CSR_STRUCT_END(mcontext)

#endif // CONFIG_RV_SDTRIG

#ifdef CONFIG_RV_IMSIC
CSR_STRUCT_START(miselect)
CSR_STRUCT_END(miselect)

CSR_STRUCT_START(mireg)
CSR_STRUCT_END(mireg)

CSR_STRUCT_START(mtopei)
  uint64_t iprio : 11; // [10: 0]
  uint64_t pad   :  5; // [15:11]
  uint64_t iid   : 11; // [26:16]
CSR_STRUCT_END(mtopei)

CSR_STRUCT_START(mtopi)
  uint64_t iprio : 8;  // [ 7: 0]
  uint64_t pad   : 8;  // [15: 8]
  uint64_t iid   : 12; // [27:16]
CSR_STRUCT_END(mtopi)

CSR_STRUCT_START(mvien)
  uint64_t pad0 : 1; // [0]
  uint64_t ssie : 1; // [1]
  uint64_t pad1 : 7; // [8:2]
  uint64_t seie : 1; // [9]
  uint64_t pad2 : 3; // [12:10]
#ifdef CONFIG_RV_SSCOFPMF
  uint64_t lcofie : 1; // [13]
#endif
CSR_STRUCT_END(mvien)

CSR_STRUCT_START(mvip)
  uint64_t pad0 : 1; // [0]
  uint64_t ssip : 1; // [1]
  uint64_t pad1 : 3; // [4:2]
  uint64_t stip : 1; // [5]
  uint64_t pad2 : 3; // [8:6]
  uint64_t seip : 1; // [9]
CSR_STRUCT_END(mvip)
#endif // CONFIG_RV_IMSIC

/* Supervisor-level CSR */

CSR_STRUCT_START(sstatus)
  uint64_t pad0 : 1;  // [0]
  uint64_t sie  : 1;  // [1]
  uint64_t pad1 : 1;  // [2]
  uint64_t spie : 1;  // [3]
  uint64_t pad2 : 1;  // [4]
  uint64_t ube  : 1;  // [5]
  uint64_t pad3 : 1;  // [6]
  uint64_t spp  : 1;  // [7]
  uint64_t vs   : 2;  // [9:8]
  uint64_t pad4 : 4;  // [13:10]
  uint64_t fs   : 2;  // [15:14]
  uint64_t xs   : 2;  // [17:16]
  uint64_t pad5 : 1;  // [18]
  uint64_t sum  : 1;  // [19]
  uint64_t mxr  : 1;  // [20]
  uint64_t pad6 : 2;  // [22:21]
  uint64_t spelp: 1;  // [23]
  uint64_t sdt  : 1;  // [24]
  uint64_t pad7 : 7;  // [31:25]
  uint64_t pad8 :16;  // [47:32]
  uint64_t uxl  : 2;  // [33:32]
  uint64_t pad9 :28;  // [61:34]
  uint64_t sd   : 1;  // [63:62]
CSR_STRUCT_END(sstatus)

typedef tvec_t stvec_t;

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
  uint64_t pad2 : 3;
#ifdef CONFIG_RV_SSCOFPMF
  uint64_t lcofie : 1;
#endif
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
  uint64_t fiom   : 1; // [0]
  uint64_t pad0   : 3; // [3:1]
  uint64_t cbie   : 2; // [5:4]
  uint64_t cbcfe  : 1; // [6]
  uint64_t cbze   : 1; // [7]
  uint64_t pad1   : 24;// [31:8]
  uint64_t pmm    : 2; // [33:32]
  uint64_t pad3   : 25;// [58:34]
  uint64_t dte    : 1; // [59]
  uint64_t cde    : 1; // [60]
  uint64_t adue   : 1; // [61]
  uint64_t pbmte  : 1; // [62]
  uint64_t stce   : 1; // [63]
CSR_STRUCT_END(senvcfg)

CSR_STRUCT_START(satp)
  uint64_t ppn :44;
  uint64_t asid:16;
  uint64_t mode: 4;
CSR_STRUCT_END(satp)

#ifdef CONFIG_RV_SSCOFPMF
CSR_STRUCT_START(scountovf)
CSR_STRUCT_END(scountovf)
#endif

/** Supervisor Custom CSRs **/

#ifdef CONFIG_RV_SVINVAL
// NOTE: srnctl is a supervisor custom read/write csr
// to fix xiangshan that:
// rnctl: move elimination,
CSR_STRUCT_START(srnctl)
  uint64_t rnctrl  : 1;
  uint64_t reserve :63;
CSR_STRUCT_END(srnctl)
#endif

CSR_STRUCT_START(sbpctl)
CSR_STRUCT_END(sbpctl)

CSR_STRUCT_START(spfctl)
CSR_STRUCT_END(spfctl)

CSR_STRUCT_START(slvpredctl)
CSR_STRUCT_END(slvpredctl)

CSR_STRUCT_START(smblockctl)
CSR_STRUCT_END(smblockctl)

CSR_STRUCT_START(sfetchctl)
CSR_STRUCT_END(sfetchctl)

/** Supervisor Timer Register**/
#ifdef CONFIG_RV_SSTC
CSR_STRUCT_START(stimecmp)
CSR_STRUCT_END(stimecmp)
#endif

/** Supervisor Advanced Interrupt Architecture CSRs **/
#ifdef CONFIG_RV_IMSIC
CSR_STRUCT_START(siselect)
CSR_STRUCT_END(siselect)
  
CSR_STRUCT_START(sireg)
CSR_STRUCT_END(sireg)

CSR_STRUCT_START(stopei)
  uint64_t iid   : 11; // [10: 0]
  uint64_t pad   :  5; // [15:11]
  uint64_t iprio : 11; // [26:16]
CSR_STRUCT_END(stopei)

CSR_STRUCT_START(stopi)
  uint64_t iprio : 8;  // [ 7: 0]
  uint64_t pad   : 8;  // [15: 8]
  uint64_t iid   : 12; // [27:16] 
CSR_STRUCT_END(stopi)
#endif // CONFIG_RV_IMSIC

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
  uint64_t usi  : 1;
  uint64_t ssi  : 1;
  uint64_t vssi : 1;
  uint64_t msi  : 1;
  uint64_t uti  : 1;
  uint64_t sti  : 1;
  uint64_t vsti : 1;
  uint64_t mti  : 1;
  uint64_t uei  : 1;
  uint64_t sei  : 1;
  uint64_t vsei : 1;
  uint64_t mei  : 1;
  uint64_t sgei : 1;
#ifdef CONFIG_RV_SSCOFPMF
  uint64_t lcofi : 1;
#endif
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
  uint64_t fiom   : 1;  // [0]
  uint64_t pad0   : 3;  // [3:1]
  uint64_t cbie   : 2;  // [5:4]
  uint64_t cbcfe  : 1;  // [6]
  uint64_t cbze   : 1;  // [7]
  uint64_t pad1   : 24; // [31:8]
  uint64_t pmm    : 2;  // [33:32]
  uint64_t pad2   : 25; // [58:34]
  uint64_t dte    : 1;  // [59]
  uint64_t pad3   : 1;  // [60]
  uint64_t adue   : 1;  // [61]
  uint64_t pbmte  : 1;  // [62]
  uint64_t stce   : 1;  // [63]
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
  uint64_t pad0   : 1;  // [0]
  uint64_t sie    : 1;  // [1]
  uint64_t pad1   : 3;  // [4:2]
  uint64_t spie   : 1;  // [5]
  uint64_t ube    : 1;  // [6]
  uint64_t pad2   : 1;  // [7]
  uint64_t spp    : 1;  // [8]
  uint64_t vs     : 2;  // [10:9]
  uint64_t pad3   : 2;  // [12:11]
  uint64_t fs     : 2;  // [14:13]
  uint64_t xs     : 2;  // [16:15]
  uint64_t pad4   : 1;  // [17]
  uint64_t sum    : 1;  // [18]
  uint64_t mxr    : 1;  // [19]
  uint64_t pad5   : 4;  // [23:20]
  uint64_t sdt    : 1;  // [24]
  uint64_t pad6   : 7;  // [31:25]
  uint64_t uxl    : 2;  // [33:32]
  uint64_t pad7   :29;  // [62:34]
  uint64_t sd     : 1;  // [63]
CSR_STRUCT_END(vsstatus)

CSR_STRUCT_START(vsie)
  uint64_t pad0 : 1;
  uint64_t ssie : 1;
  uint64_t pad1 : 3;
  uint64_t stie : 1;
  uint64_t pad2 : 3;
  uint64_t seie : 1;
  uint64_t pad3 : 3;
#ifdef CONFIG_RV_SSCOFPMF
  uint64_t lcofie : 1;
#endif
CSR_STRUCT_END(vsie)

typedef tvec_t vstvec_t;

CSR_STRUCT_START(vsscratch)
CSR_STRUCT_END(vsscratch)

CSR_STRUCT_START(vsepc)
CSR_STRUCT_END(vsepc)

CSR_STRUCT_START(vscause)
  uint64_t code :63;
  uint64_t intr : 1;
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

/** Virtual Supervisor Timer Register **/
#ifdef CONFIG_RV_SSTC
CSR_STRUCT_START(vstimecmp)
CSR_STRUCT_END(vstimecmp)
#endif

CSR_STRUCT_START(vsatp)
  uint64_t ppn  :44;
  uint64_t asid :16;
  uint64_t mode : 4;
CSR_STRUCT_END(vsatp)

#endif // CONFIG_RVH

/** Hypervisor and VS AIA CSRs **/
#ifdef CONFIG_RV_IMSIC
CSR_STRUCT_START(hvien)
  uint64_t pad    : 13;
#ifdef CONFIG_RV_SSCOFPMF
  uint64_t lcofie : 1;
#endif
CSR_STRUCT_END(hvien)

CSR_STRUCT_START(hvictl)
  uint64_t iprio  : 8;  // [7:0]
  uint64_t ipriom : 1;  // [8]
  uint64_t dpr    : 1;  // [9]
  uint64_t pad0   : 6;  // [15:10]
  uint64_t iid    : 12; // [27:16]
  uint64_t pad1   : 2;  // [29:28]
  uint64_t vti    : 1;  // [30]
CSR_STRUCT_END(hvictl)

CSR_STRUCT_START(hviprio1)
CSR_STRUCT_END(hviprio1)

CSR_STRUCT_START(hviprio2)
CSR_STRUCT_END(hviprio2)

CSR_STRUCT_START(vsiselect)
CSR_STRUCT_END(vsiselect)

CSR_STRUCT_START(vsireg)
CSR_STRUCT_END(vsireg)

CSR_STRUCT_START(vstopei)
  uint64_t iid   : 11; // [10: 0]
  uint64_t pad   :  5; // [15:11]
  uint64_t iprio : 11; // [26:16]
CSR_STRUCT_END(vstopei)

CSR_STRUCT_START(vstopi)
  uint64_t iprio : 8;  // [ 7: 0]
  uint64_t pad   : 8;  // [15: 8]
  uint64_t iid   : 12; // [27:16] 
CSR_STRUCT_END(vstopi)
#endif // CONFIG_RV_IMSIC

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
rtlreg_t get_mask(int reg, int idx);
void set_mask(uint32_t reg, int idx, uint64_t mask, uint64_t vsew, uint64_t vlmul);

#endif // CONFIG_RVV

#ifdef CONFIG_RV_ZICNTR
CSR_STRUCT_START(cycle)
CSR_STRUCT_END(cycle)

#ifdef CONFIG_RV_CSR_TIME
CSR_STRUCT_START(csr_time)
CSR_STRUCT_END(csr_time)
#endif // CONFIG_RV_CSR_TIME

CSR_STRUCT_START(instret)
CSR_STRUCT_END(instret)
#endif // CONFIG_RV_ZICNTR

#ifdef CONFIG_RV_ZIHPM
CSR_STRUCT_DUMMY_LIST(CSRS_UNPRIV_HPMCOUNTER)
#endif // CONFIG_RV_ZIHPM

/**  Machine Non-Maskable Interrupt Handling **/
#ifdef CONFIG_RV_SMRNMI
CSR_STRUCT_START(mnepc)
CSR_STRUCT_END(mnepc)

CSR_STRUCT_START(mncause)
CSR_STRUCT_END(mncause)

CSR_STRUCT_START(mnstatus)
  uint64_t pad0   : 3;  // [2:0]
  uint64_t nmie   : 1;  // [3]
  uint64_t pad1   : 3;  // [6:4]
  uint64_t mnpv   : 1;  // [7]
  uint64_t pad2   : 1;  // [8]
  uint64_t mnpelp : 1;  // [9]
  uint64_t pad3   : 1;  // [10]
  uint64_t mnpp   : 2;  // [12:11]
  uint64_t pad4   : 51; // [63:13]
CSR_STRUCT_END(mnstatus)

CSR_STRUCT_START(mnscratch)
CSR_STRUCT_END(mnscratch)
#endif // CONFIG_RV_SMRNMI


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
#define SATP_MODE_Sv48 9
#define SATP_ASID_LEN 16 // max is 16
#define SATP_PADDR_LEN (CONFIG_PADDRBITS-12) // max is 44
#define SATP_ASID_MAX_LEN 16
#define SATP_PADDR_MAX_LEN 44

#define SATP_MODE39_MASK (8UL << (SATP_ASID_MAX_LEN + SATP_PADDR_MAX_LEN))
#define SATP_MODE48_MASK (9UL << (SATP_ASID_MAX_LEN + SATP_PADDR_MAX_LEN))
#define SATP_ASID_MASK (((1L << SATP_ASID_LEN)-1) << SATP_PADDR_MAX_LEN)
#define SATP_PADDR_MASK ((1L << SATP_PADDR_LEN)-1)

#ifdef CONFIG_RV_SV48
#define SATP_MASK (SATP_MODE39_MASK | SATP_MODE48_MASK | SATP_ASID_MASK | SATP_PADDR_MASK)
#else
#define SATP_MASK (SATP_MODE39_MASK | SATP_ASID_MASK | SATP_PADDR_MASK)
#endif // CONFIG_RV_SV48
#define MASKED_SATP(x) (SATP_MASK & x)

/** CSR hgatp **/
#ifdef CONFIG_RVH
#define HGATP_MODE_BARE   0
#define HGATP_MODE_Sv39x4 8
#define HGATP_MODE_Sv48x4 9
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
#define HGATP_Sv48x4_GPADDR_LEN 50
#define VSATP_PPN_HGATP_BARE_MASK BITMASK(HGATP_Bare_GPADDR_LEN - PAGE_SHIFT)
#define VSATP_PPN_HGATP_Sv39x4_MASK BITMASK(HGATP_Sv39x4_GPADDR_LEN - PAGE_SHIFT)
#define VSATP_PPN_HGATP_Sv48x4_MASK BITMASK(HGATP_Sv48x4_GPADDR_LEN - PAGE_SHIFT)
#endif // CONFIG_RVH

/** RVH **/
#ifdef CONFIG_RVH
  extern bool v; // virtualization mode
#endif // CONFIG_RVH



/** SSTATUS **/
// All valid fields defined by RISC-V spec and not affected by extensions
// This mask is used to get the value of sstatus from mstatus
// SD, SDT, UXL, MXR, SUM, XS, FS, VS, SPP, UBE, SPIE, SIE
#define SSTATUS_BASE 0x80000003000de762UL

#define SSTATUS_SDT MUXDEF(CONFIG_RV_SMRNMI, 0x1000000, 0)

#define SSTATUS_RMASK (SSTATUS_BASE | SSTATUS_SDT)

/** AIA **/
#ifdef CONFIG_RV_IMSIC
  #define ISELECT_2F_MASK 0x2F
  #define ISELECT_3F_MASK 0x3F
  #define ISELECT_6F_MASK 0x6F
  #define ISELECT_7F_MASK 0x7F
  #define ISELECT_MAX_MASK 0xFF
  #define VSISELECT_MAX_MASK 0x1FF
#endif // CONFIG_RV_IMSIC

/** Double Trap**/
#ifdef CONFIG_RV_SMRNMI
  #define MNSTATUS_NMIE   0x1UL << 3
  #define MNSTATUS_MNPV   0X1UL << 7
  #define MNSTATUS_MNPELP 0X1UL << 9
  #define MNSTATUS_MNPP   0X3UL << 11
  #define MNSTATUS_MASK (MNSTATUS_NMIE | MNSTATUS_MNPV | MNSTATUS_MNPP)
#endif

/**
 * Function declaration
*/

/** General **/
void csr_prepare();

word_t gen_status_sd(word_t status);
word_t get_mip();

/** PMP **/
uint8_t pmpcfg_from_index(int idx);
word_t pmpaddr_from_index(int idx);
word_t pmp_tor_mask();

#ifndef CSR_ENCODING_H
#define CSR_ENCODING_H
// Unprivileged and User-Level CSRs
#define CSR_FFLAGS 0x1
#define CSR_FRM 0x2
#define CSR_FCSR 0x3
#define CSR_VXSTART 0x8
#define CSR_VXSAT 0x9
#define CSR_VXRM 0xa
#define CSR_VCSR 0xf
#define CSR_CYCLE 0xc00
#define CSR_CSR_TIME 0xc01
#define CSR_INSTRET 0xc02
#define CSR_VLENB 0xc22
// Supervisor-Level CSRs
#define CSR_SSTATUS 0x100
#define CSR_SCOUNTEREN 0x106
#define CSR_SENVCFG 0x10a
#define CSR_SSTATEEN0 0x10c
#define CSR_SIE 0x104
#define CSR_STVEC 0x105
#define CSR_SSCRATCH 0x140
#define CSR_SEPC 0x141
#define CSR_SCAUSE 0x142
#define CSR_STVAL 0x143
#define CSR_SIP 0x144
#define CSR_STIMECMP 0x14d
#define CSR_SATP 0x180
#define CSR_SCOUNTOVF 0xda0
#define CSR_STOPI 0xdb0
// Hypervisor and VS CSRs
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
#define CSR_HCOUNTEREN 0x606
#define CSR_HGEIE 0x607
#define CSR_HVIEN 0x608
#define CSR_HENVCFG 0x60a
#define CSR_HSTATEEN0 0x60c
#define CSR_HGATP 0x680
#define CSR_HIP 0x644
#define CSR_HVIP 0x645
#define CSR_HGEIP 0xe12
#define CSR_VSTOPI 0xeb0
// Machine-Level CSRs
#define CSR_MSTATUS 0x300
#define CSR_MISA 0x301
#define CSR_MEDELEG 0x302
#define CSR_MIDELEG 0x303
#define CSR_MIE 0x304
#define CSR_MTVEC 0x305
#define CSR_MCOUNTEREN 0x306
#define CSR_MVIEN 0x308
#define CSR_MVIP 0x309
#define CSR_MENVCFG 0x30a
#define CSR_MSTATEEN0 0x30c
#define CSR_MCOUNTINHIBIT 0x320
#define CSR_MEPC 0x341
#define CSR_MIP 0x344
#define CSR_MNSCRATCH 0x740
#define CSR_MNEPC 0x741
#define CSR_MNCAUSE 0x742
#define CSR_MNSTATUS 0x744
#define CSR_TSELECT 0x7a0
#define CSR_TDATA1 0x7a1
#define CSR_TDATA2 0x7a2
#define CSR_TDATA3 0x7a3
#define CSR_MCYCLE 0xb00
#define CSR_MINSTRET 0xb02
#define CSR_MTOPI 0xfb0
#endif // CSR_ENCODING_H

#endif // __CSR_H__
