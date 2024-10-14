/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* DiffTest is licensed under Mulan PSL v2.
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
#ifndef __TRACE_FORMAT_H__
#define __TRACE_FORMAT_H__

#include <cstdint>
#include <stdio.h>
#include <trace/spikedasm.h>
#include <trace/trace_common.h>


// TODO : pack it
// TODO : mv instr and instr_pc to map(or more ca, icache)

enum MemoryType {
  MEM_TYPE_None = 0,
  MEM_TYPE_Load = 1,
  MEM_TYPE_Store = 2,
  MEM_TYPE_AMO = 3,
  MEM_TYPE_LR = 4,
  MEM_TYPE_SC = 5
};

enum MemorySize {
  MEM_SIZE_Byte = 0,
  MEM_SIZE_Half = 1,
  MEM_SIZE_Word = 2,
  MEM_SIZE_Double = 3
};

enum BranchType {
  BRANCH_None = 0,
  BRANCH_Cond = 1,
  BRANCH_Uncond = 2,
  BRANCH_Call = 3,
  BRANCH_Return = 4
};

enum CFIResult {
  Sequence = 0,
  BRANCH_Taken = 1,
  EXCEPTION = 2,
};

// union doest allow non-trivial constructor
struct MemoryAddr {
  uint64_t va;
  uint64_t pa;
};

// union doest allow non-trivial constructor
struct ArthiSrc {
  uint64_t src0;
  uint64_t src1;
};

struct TraceInstruction {
  uint64_t instr_pc_va = 0;
  uint64_t instr_pc_pa = 0;
  union {
    MemoryAddr memory_address;
    ArthiSrc arthi_src;
  } exu_data;
  uint64_t target = 0;
  uint32_t instr = 0;

  uint8_t memory_type:4; // lsb
  uint8_t memory_size:4; // msb

  uint8_t branch_type = 0;
  uint8_t taken = 0;
  uint8_t exception = 0;
  // uint8_t padding = 0xff;

  void dump() {
    // printf("Instr: TraceSize %ld memSize %02x PC 0x%016lx instr 0x%04x memAddr 0x%016lx\n", sizeof(TraceInstruction), memory_size, instr_pc, instr, memory_address);
    printf("PC 0x%08lx|%08lx instr 0x%08x(%s)", instr_pc_va, instr_pc_pa, instr, spike_dasm(instr));
    if (memory_type != MEM_TYPE_None) {
      printf(" is_mem %d addr %08lx|%08lx", memory_type, exu_data.memory_address.va, exu_data.memory_address.pa);
    } else {
      printf(" arthi([f]div/sqrt) src0 %016lx src1 %016lx", exu_data.arthi_src.src0, exu_data.arthi_src.src1);
    }
    if (branch_type != BRANCH_None) {
      printf(" branch_type %d taken %d target %08lx", branch_type, taken, target);
    }
    if (exception != 0) {
      printf(" excep %d target %08lx", exception, target);
    }
    printf("\n");
  }
  void dumpWithID(uint64_t id) {
    printf("[%08lx] ", id);
    dump();
  }
};

struct Instruction : TraceInstruction {};

struct Control {
  uint8_t type;
  uint8_t data;
  // TODO: placeholder, not implemented
};

struct TracePageEntry {
  uint64_t paddr;
  uint64_t pte;
};

typedef union TracePageTableEntry {
  struct  {
    uint8_t v : 1;
    uint8_t r : 1;
    uint8_t w : 1;
    uint8_t x : 1;
    uint8_t u : 1;
    uint8_t g : 1;
    uint8_t a : 1;
    uint8_t d : 1;
    uint8_t rsw : 2;
    uint64_t ppn : 44;
    uint8_t reserved : 7;
    uint8_t pbmt : 2;
    uint8_t n : 1;
  };
  uint64_t val;
} TracePTE;

#endif