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

#include <stdexcept>
#include <sstream>
#include <map>
#include <iostream>
#include <trace/trace_writer.h>

TraceWriter::TraceWriter(std::string trace_file_name) {
  trace_stream = new std::ofstream(trace_file_name, std::ios_base::out);
  if ((!trace_stream->is_open())) {
    std::ostringstream oss;
    oss << "[TraceWriter.TraceWriter] Could not open file: " << trace_file_name;
    // throw std::runtime_error(oss.str());
  }
}

bool TraceWriter::write(Instruction &inst) {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.write] empty trace_stream.");
    return false;
  }

  trace_stream->write(reinterpret_cast<char *> (&inst), sizeof(Instruction));
  return true;
}

bool TraceWriter::write(uint64_t pc, uint32_t instr) {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.write] empty trace_stream.");
    return false;
  }

  Instruction inst;
  inst.instr_pc = pc;
  inst.instr = instr;
  inst.padding = 0;
  // inst.memory_size = 0;
  // inst.memory_address = 0;

  trace_stream->write(reinterpret_cast<char *> (&inst), sizeof(Instruction));
  return true;
}

bool TraceWriter::write(Control &ctrl) {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.write] empty trace_stream.");
    return false;
  }

  trace_stream->write(reinterpret_cast<char *> (&ctrl), sizeof(Control));
  return true;
}

void TraceWriter::traceOver() {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.traceOver] empty trace_stream.");
    return;
  }

  trace_stream->close();
}

std::map<uint64_t, uint64_t> pc_map_count;

void pcMapCountLog(uint64_t pc) {
  if (pc_map_count.find(pc) == pc_map_count.end()) {
    pc_map_count[pc] = 1;
  } else {
    pc_map_count[pc] += 1;
  }
}

void pcMapCountPrint() {
  std::cout << "MAP: pc_map_count" << std::endl;
  for (auto it = pc_map_count.begin(); it != pc_map_count.end(); it++) {
    std::cout << std::hex << it->first
              << std::dec << " -> " << it->second
              << std::endl;
  }
}


TraceWriter *trace_writer = new TraceWriter("Im-A-TraceName");

extern "C"  {

// void init_trace_writer(std::string trace_file_name) {
//   trace_writer = new TraceWriter(trace_file_name);
// }

void trace_write(uint64_t pc, uint32_t instr) {
  Instruction inst;
  inst.instr_pc = pc;
  inst.instr = instr;
  inst.padding = 0;
  trace_writer->write(inst);
}

void trace_end() {
  trace_writer->traceOver();
  delete trace_writer;
}

void pc_map_count_log(uint64_t pc) {
  pcMapCountLog(pc);
}

void pc_map_count_print() {
  pcMapCountPrint();
}

}