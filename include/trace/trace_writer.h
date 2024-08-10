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
#ifndef __TRACE_WRITER_H__
#define __TRACE_WRITER_H__

#include <fstream>
#include <deque>
#include <string>
#include "trace_format.h"

// struct {
//   bool valid;
//   bool pc_va; // get pc va
//   bool pc_pa; // get pc pa
//   bool fetched; // get instruction code
//   bool decoded; // decoded
//   bool branched; // get the branch result
//   // bool executed; // arthimetic/float/branch executed. not record.
//   bool memory_va; // get memory va
//   bool memory_pa; // get memory pa
//   bool memory_accessed; // memory access
//   bool over; // instruction over
// } InstrStatus;

class TraceWriter {
  std::ofstream *trace_stream;

  uint64_t instCounter = 0;
  bool inst_valid = false;
  Instruction inst;
  uint64_t tmp_priv_target = 0;

  const uint32_t instListSize = 16;
  std::deque<Instruction> inst_list;

public:
  TraceWriter(std::string trace_file_name);
  ~TraceWriter() {
    delete trace_stream;
  }

  /* write an instruction */
  bool write(Instruction &inst);
  /* write an control */
  bool write(Control &ctrl);

  // InstStatus instStatus;

  void inst_start();
  void write_pc(uint64_t pc);
  void write_inst(uint32_t instr);
  void write_pc_pa(uint64_t pa);
  void write_memory(uint64_t va, uint8_t size, uint8_t is_write);
  void write_mem_pa(uint64_t pa);
  void write_branch(uint64_t target, uint8_t branch_type, uint8_t is_taken);
  void write_exception(uint8_t NO, uint64_t target);
  void write_interrupt(uint8_t NO, uint64_t target);
  void inst_over();
  void inst_reset();

  void error_dump();

  void traceOver();
};

extern TraceWriter *trace_writer;

#endif