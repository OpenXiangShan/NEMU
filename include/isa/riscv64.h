#ifndef __ISA_RISCV64_H__
#define __ISA_RISCV64_H__

#include <common.h>

// memory
#ifdef __ENGINE_rv64__
#define riscv64_IMAGE_START 0x100000
#else
#define riscv64_IMAGE_START 0x0
#endif
#define riscv64_PMEM_BASE 0x80000000

// #define XIANGSHAN_DEBUG
// #define ENABLE_DISAMBIGUATE
#define FORCE_RAISE_PF

// reg
struct DisambiguationState {
  uint64_t exceptionNo;
  uint64_t mtval;
  uint64_t stval;
};

typedef struct {
  union {
    uint64_t _64;
  } gpr[32];

  union {
    uint64_t _64;
  } fpr[32];

  uint64_t pc;
  uint64_t mstatus, mcause, mepc;
  uint64_t sstatus, scause, sepc;

  uint64_t satp, mip, mie, mscratch, sscratch, mideleg, medeleg;

  uint64_t mtval, stval, mtvec, stvec;
  uint64_t mode;

  bool amo;
  int mem_exception;

  // for LR/SC
  uint64_t lr_addr;
  uint64_t lr_valid;

  bool INTR;

  // Disambiguation
  bool need_disambiguate;
  struct DisambiguationState disambiguation_state;
} riscv64_CPU_state;

// decode
typedef struct {
  union {
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      uint32_t funct7    : 7;
    } r;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      int32_t  simm11_0  :12;
    } i;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t imm4_0    : 5;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      int32_t  simm11_5  : 7;
    } s;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t imm11     : 1;
      uint32_t imm4_1    : 4;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      uint32_t imm10_5   : 6;
      int32_t  simm12    : 1;
    } b;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      int32_t  simm31_12 :20;
    } u;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t imm19_12  : 8;
      uint32_t imm11     : 1;
      uint32_t imm10_1   :10;
      int32_t  simm20    : 1;
    } j;
    struct {
      uint32_t pad7      :20;
      uint32_t csr       :12;
    } csr;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t rm        : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      uint32_t fmt       : 2;
      uint32_t funct5    : 5;
    } fp;
    uint32_t val;
  } instr;
} riscv64_ISADecodeInfo;

#define TLBEntryNum 32
#define TLBSPEntryNum 4
#define L2TLBL1EntryNum 16
#define L2TLBL2EntryNum 1024
#define L2TLBL3EntryNum 4096
#define L2TLBSPEntryNum 16
#define L2TLBL2WayNum 8
#define L2TLBL3WayNum 16
#define L2TLBWaySize 4
#define L2TLBL2SetNum (L2TLBL2EntryNum / L2TLBL2WayNum / L2TLBWaySize)
#define L2TLBL3SetNum (L2TLBL3EntryNum / L2TLBL3WayNum / L2TLBWaySize)
#define EntryNumPerWalker 8
#define MAXSTRIDE 4
// NOTE: 只需要stride为4即可
#define DOOLDHEBING

// just record miss rate, don't do the translation job
typedef struct {
  uint64_t tag;
  bool v;
} tlb_entry;

typedef struct {
  uint64_t tag;
  bool v;
  uint64_t size;
} tlb_sp_entry;

typedef struct {
  uint64_t tag;
  bool v;
  uint64_t length;
  uint64_t ppn;
  uint64_t stride;
} tlb_hb_entry;

typedef struct {
  // tlb_entry normal[TLBEntryNum];
  tlb_hb_entry hebing[TLBEntryNum];
  tlb_sp_entry super[TLBSPEntryNum];

  uint64_t access;
  uint64_t miss;

  uint64_t hb_new[EntryNumPerWalker];
  uint64_t hb_old[EntryNumPerWalker + EntryNumPerWalker + 1];
  uint64_t hb_access[EntryNumPerWalker + EntryNumPerWalker + 1];
  uint64_t hb_stride_new[EntryNumPerWalker];
  uint64_t hb_stride_old[EntryNumPerWalker + EntryNumPerWalker + 1];
  uint64_t hb_stride_access[EntryNumPerWalker + EntryNumPerWalker + 1];
} riscv64_TLB_State;

typedef struct {
  tlb_entry l1[L2TLBL1EntryNum];
  tlb_entry l2[L2TLBL2SetNum][L2TLBL2WayNum];
  tlb_entry l3[L2TLBL3SetNum][L2TLBL3WayNum];
  tlb_sp_entry sp[L2TLBSPEntryNum];

  uint64_t access;
  uint64_t miss;
  uint64_t mem_access;
} riscv64_L2TLB_State;

#define riscv64_has_mem_exception() (cpu.mem_exception != 0)

enum {PAGE_4KB, PAGE_2MB, PAGE_1GB};
#define VPN(vaddr) (vaddr >> 12)
#define SUPERVPN(vaddr, i) (VPN(vaddr) >> ((3-i) * 9))
#define get_l3_index(vaddr) ((VPN(vaddr) / L2TLBWaySize) % L2TLBL3SetNum)
#define get_l2_index(vaddr) (((VPN(vaddr) >> 9) / L2TLBWaySize) % L2TLBL2SetNum)
#define get_l3_tag(vaddr) ((VPN(vaddr) / L2TLBWaySize))
#define get_l2_tag(vaddr) ((VPN(vaddr) >> 9) / L2TLBWaySize)
#define get_l1_tag(vaddr) ((VPN(vaddr) >> 18))
void riscv64_tlb_access(uint64_t vaddr, uint64_t type);
void mmu_statistic();

// #define TLB_DEBUG

#endif
