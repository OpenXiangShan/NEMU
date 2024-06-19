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

// #define Log() printf("file: %s, line: %d\n", __FILE__, __LINE__); fflush(stdout)

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

enum BrachTaken {
  BRANCH_NotTaken = 0,
  BRANCH_Taken = 1
};

struct TraceInstruction {
  uint64_t instr_pc_va = 0;
  uint64_t instr_pc_pa = 0;
  uint64_t memory_address_va = 0;
  uint64_t memory_address_pa = 0;
  uint64_t target = 0;
  uint32_t instr = 0;
  uint8_t memory_type = 0;
  uint8_t memory_size = 0;
  uint8_t branch_type = 0;
  uint8_t taken = 0;
  // uint8_t padding = 0xff;

  void dump() {
    // printf("Instr: TraceSize %ld memSize %02x PC 0x%016lx instr 0x%04x memAddr 0x%016lx\n", sizeof(TraceInstruction), memory_size, instr_pc, instr, memory_address);
    printf("Instr: size %ld PC 0x%08lx|%08lx instr 0x%08x", sizeof(TraceInstruction), instr_pc_va, instr_pc_pa, instr);
    if (memory_type != MEM_TYPE_None) {
      printf(" is_mem %d addr %08lx|%08lx", memory_type, memory_address_va, memory_address_pa);
    }
    if (branch_type != BRANCH_None) {
      printf(" is_branch %d taken %d target %08lx", branch_type, taken, target);
    }
    printf("\n");
  }
};

struct Instruction : TraceInstruction {};

struct Control {
  uint8_t type;
  uint8_t data;
  // TODO: placeholder, not implemented
};

#endif