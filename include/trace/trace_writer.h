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
#include "trace_format.h"

class TraceWriter {
  std::ofstream *trace_stream;

public:
  TraceWriter(std::string trace_file_name);
  ~TraceWriter() {
    delete trace_stream;
  }

  /* write an instruction */
  bool write(Instruction &inst);
  /* write an bare instruction */
  bool write(uint64_t pc, uint32_t instr);
  /* write an control */
  bool write(Control &ctrl);

  void traceOver();
};

extern TraceWriter *trace_writer;

#endif