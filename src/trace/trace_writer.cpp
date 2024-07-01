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
#include <cstring>
#include <map>
#include <iostream>
#include <trace/trace_writer.h>

// #define TraceLog(...) printf(__VA_ARGS__); fflush(stdout)
#define TraceLog(...)

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

bool TraceWriter::write(Control &ctrl) {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.write] empty trace_stream.");
    return false;
  }

  TraceLog("TraceWriter::write\n");
  trace_stream->write(reinterpret_cast<char *> (&ctrl), sizeof(Control));
  return true;
}

void TraceWriter::inst_start() {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.inst_start] empty trace_stream.");
    return;
  }
  if (inst_valid) {
    throw std::runtime_error("[TraceWriter.inst_start] inst_valid already true.");
    return;
  }

  TraceLog("TraceWriter::inst_start\n");
  inst_valid = true;
  return;
}

void TraceWriter::inst_over() {
  if (!inst_valid) {
    throw std::runtime_error("[TraceWriter.inst_over] inst_valid already false.");
    return;
  }

  trace_stream->write(reinterpret_cast<char *> (&inst), sizeof(Instruction));
  inst_list.push_back(inst);

  TraceLog("TraceWriter::inst_over\n");
  inst_valid = false;
  return;
}

void TraceWriter::inst_reset() {
  TraceLog("TraceWriter::inst_reset\n");
  memset(reinterpret_cast<void*>(&inst), 0, sizeof(Instruction));
}

void TraceWriter::write_pc(uint64_t pc) {
  if (!inst_valid) {
    throw std::runtime_error("[TraceWriter.write_pc] inst_valid false.");
    return;
  }
  TraceLog("TraceWriter::write_pc\n");
  inst.instr_pc_va = pc;
  return;
}

void TraceWriter::write_inst(uint32_t instr) {
  if (!inst_valid) {
    throw std::runtime_error("[TraceWriter.write_inst] inst_valid false.");
    return;
  }

  TraceLog("TraceWriter::write_inst\n");
  inst.instr = instr;
  return;
}

void TraceWriter::write_memory(uint64_t va, uint8_t size, uint8_t is_write) {
  if (!inst_valid) {
    throw std::runtime_error("[TraceWriter.write_mem] inst_valid false.");
    return;
  }
  // 1, 2, 4, 8, manual log2
  uint8_t size_map[] = {0, 0, 1, 0, 2, 0, 0, 0, 3};

  TraceLog("TraceWriter::write_memory\n");
  inst.memory_address_va = va;
  inst.memory_size = size_map[size];
  inst.memory_type = is_write ? MEM_TYPE_Store : MEM_TYPE_Load;
  return;
}

void TraceWriter::write_branch(uint64_t target, uint8_t branch_type, uint8_t is_taken) {
  if (!inst_valid) {
    throw std::runtime_error("[TraceWriter.write_branch] inst_valid false.");
    return;
  }

  if (inst.branch_type == BRANCH_None) {
    TraceLog("TraceWriter::write_branch/jump target\n");
    inst.target = target;
    inst.branch_type = branch_type;
    inst.taken = is_taken;
  } else {
    TraceLog("TraceWriter::write_branch target(nested call rtl_j)\n");
  }
  return;
}

void TraceWriter::write_pc_pa(uint64_t pa) {
  if (!inst_valid) {
    throw std::runtime_error("[TraceWriter.write_pc_pa] inst_valid false.");
    return;
  }

  TraceLog("TraceWriter::write_pc_pa\n");
  inst.instr_pc_pa = pa;
  return;
}

void TraceWriter::write_mem_pa(uint64_t pa) {
  if (!inst_valid) {
    throw std::runtime_error("[TraceWriter.write_mem_pa] inst_valid false.");
    return;
  }

  TraceLog("TraceWriter::write_mem_pa\n");
  inst.memory_address_pa = pa;
  return;
}


void TraceWriter::traceOver() {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.traceOver] empty trace_stream.");
    return;
  }

  TraceLog("TraceWriter::traceOver\n");

  // for (auto it = inst_list.begin(); it != inst_list.end(); it++) {
  //   it->dump();
  // }

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

void trace_write_base(uint64_t pc) {
  trace_writer->inst_reset();
  trace_writer->inst_start();
  trace_writer->write_pc(pc);
}

void trace_write_pc_pa(uint64_t pa) {
  trace_writer->write_pc_pa(pa);
}

void trace_write_inst(uint32_t instr) {
  trace_writer->write_inst(instr);
}

void trace_write_memory(uint64_t va, uint8_t size, uint8_t is_write) {
  trace_writer->write_memory(va, size, is_write);
}

void trace_write_mem_pa(uint64_t pa) {
  trace_writer->write_mem_pa(pa);
}

void trace_write_branch(uint64_t target, uint8_t branch_type, uint8_t is_taken) {
  trace_writer->write_branch(target, branch_type, is_taken);
}

void trace_inst_over() {
  trace_writer->inst_over();
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