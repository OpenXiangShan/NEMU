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
// #include <boost/stacktrace.hpp>
#include <zstd.h>

// #define VERBOSE
// #define LogBuffer
#define MORECHECK

#ifdef LogBuffer

#define TraceLogEntryNum 16
#define TraceLogEntrySize 256
char trace_log_buf[TraceLogEntryNum][TraceLogEntrySize];
uint8_t trace_log_ptr = 0;

uint8_t trace_log_ptr_pop() {
  return (trace_log_ptr ++) % TraceLogEntrySize;
}
// uint8_t trace_log_ptr_get() {
//   return trace_log_ptr % TraceLogEntrySize;
// }
#endif

#ifdef LogBuffer
#define WHEREAMI snprintf(trace_log_buf[trace_log_ptr_pop()], TraceLogEntrySize, "FromBuf:%s:%d\n", __FILE__, __LINE__);
#else
#define WHEREAMI printf("%s:%d:%s\n", __FILE__, __LINE__, __func__); fflush(stdout);
#endif

#define WHEREAMIDirect printf("Direct:%s:%d:%s\n", __FILE__, __LINE__, __func__); fflush(stdout);

#ifdef VERBOSE
#define TraceSimpleLog WHEREAMI

#ifdef LogBuffer
#define TraceLog(...) snprintf(trace_log_buf[trace_log_ptr_pop()], TraceLogEntrySize, __VA_ARGS__);
#else
#define TraceLog(...) printf(__VA_ARGS__); fflush(stdout);
#endif

#else
#define TraceSimpleLog
#define TraceLog(...)
#endif

#ifdef MORECHECK
#define TraceRequireInvalid                     \
  do {                                       \
    if (inst_valid) {                        \
      error_dump();                          \
      WHEREAMIDirect;                        \
      exit(1);                               \
    }                                        \
  } while(0);

#define TraceRequireValid                   \
  do {                                       \
    if (!inst_valid) {                       \
      error_dump();                          \
      WHEREAMIDirect;                        \
      exit(1);                               \
    }                                        \
  } while(0);
#else
#define TraceRequireInvalid
#define TraceRequireValid
#endif

TraceWriter::TraceWriter(std::string trace_file_name) {
  trace_stream = new std::ofstream(trace_file_name, std::ios_base::out);
  if ((!trace_stream->is_open())) {
    std::ostringstream oss;
    oss << "[TraceWriter.TraceWriter] Could not open file: " << trace_file_name;
    // throw std::runtime_error(oss.str());
  }

  instBuffer = (Instruction *)  malloc(sizeof(Instruction)* MAX_INSTRUCTION_NUM);
  if (instBuffer == NULL) {
    printf("Error: malloc instBuffer(%lx)\n", sizeof(Instruction)* MAX_INSTRUCTION_NUM);
    exit(0);
  }
}

void TraceWriter::inst_start() {
  TraceSimpleLog;
  TraceRequireInvalid;
  inst_valid = true;
  return;
}

void TraceWriter::inst_over() {
  TraceSimpleLog;
  TraceRequireValid;

  instBuffer[instBufferPtr++] = inst;
  // trace_stream->write(reinterpret_cast<char *> (&inst), sizeof(Instruction));

  // inst.dumpWithID(instBufferPtr);

  if (instListSize <= inst_list.size()) {
    inst_list.pop_front();
  }
  inst_list.push_back(inst);

  inst_valid = false;

  if (instBufferPtr >= MAX_INSTRUCTION_NUM) {
    printf("Traced Instructin Number enough(%ld), exit.", instBufferPtr);
    traceOver();
    exit(0);
  }
  return;
}

void TraceWriter::inst_reset() {
  TraceSimpleLog;
  TraceRequireInvalid;
  memset(reinterpret_cast<void*>(&inst), 0, sizeof(Instruction));
}

void TraceWriter::write_pc(uint64_t pc) {
  TraceSimpleLog;
  TraceRequireValid;
  inst.instr_pc_va = pc;
  return;
}

void TraceWriter::write_pc_pa(uint64_t pa) {
  TraceSimpleLog;
  TraceRequireValid;

  inst.instr_pc_pa = pa;
  return;
}

void TraceWriter::write_inst(uint32_t instr) {
  TraceSimpleLog;
  TraceRequireValid;
  inst.instr = instr;
  return;
}

void TraceWriter::write_memory(uint64_t va, uint8_t size, uint8_t is_write) {
  TraceSimpleLog;
  TraceRequireValid;

  // 1, 2, 4, 8, manual log2
  uint8_t size_map[] = {0, 0, 1, 0, 2, 0, 0, 0, 3};

  // printf("write_memory: va=%lx, size=%d, is_write=%d\n", va, size, is_write);

  inst.exu_data.memory_address.va = va;
  inst.memory_size = size_map[size];
  inst.memory_type = is_write ? MEM_TYPE_Store : MEM_TYPE_Load;
  return;
}

void TraceWriter::write_mem_pa(uint64_t pa) {
  TraceSimpleLog;
  TraceRequireValid;

  // printf("write_mem_pa: pa=%lx\n", pa);

  inst.exu_data.memory_address.pa = pa;
  return;
}

void TraceWriter::write_arthi_src(uint64_t src0, uint64_t src1) {
  TraceSimpleLog;
  TraceRequireValid;

  // printf("write_arthi_src: src0=%lx, src1=%lx\n", src0, src1);

  inst.exu_data.arthi_src.src0 = src0;
  inst.exu_data.arthi_src.src1 = src1;
  return;
}

void TraceWriter::write_branch(uint64_t target, uint8_t branch_type, uint8_t is_taken) {
  TraceSimpleLog;
  TraceRequireValid;

  if (inst.branch_type == BRANCH_None) {
    TraceSimpleLog;
    inst.target = target;
    inst.branch_type = branch_type;
    inst.taken = is_taken;
  } else {
    TraceSimpleLog;
  }
  return;
}

void TraceWriter::write_exception(uint8_t NO, uint64_t target) {
  TraceSimpleLog;
  TraceRequireValid;

  inst.exception = NO;
  inst.target = target;
}

void TraceWriter::write_interrupt(uint8_t NO, uint64_t target) {
  TraceSimpleLog;
  TraceRequireInvalid;

  // NEMU's interrupt doest "rely" on a instruction. So we can not get the inst info.
  // Make an interrupt a arbitrary jump
  inst_reset();
  inst_start();
  inst.exception = 0x80 | NO;
  inst.target = target;
}


void TraceWriter::traceOver() {
  if (trace_stream == NULL) {
    error_dump();
    throw std::runtime_error("[TraceWriter.traceOver] empty trace_stream.");
    return;
  }

  uint64_t instBufferSize = sizeof(Instruction) * instBufferPtr;
  char *compressedBuffer = (char *)malloc(instBufferSize);

  uint64_t compressedSize = compressZSTD(compressedBuffer, sizeof(Instruction) * instBufferPtr, (char *)instBuffer, instBufferSize);

  trace_stream->write(compressedBuffer, compressedSize);
  // for (uint64_t idx = 0; idx < instBufferPtr; idx++) {
  //   trace_stream->write((char *)&instBuffer[idx], sizeof(instBuffer[idx]));
  // }

  // for (uint64_t i = 0; i < instBufferPtr; i++) {
  //   instBuffer[i].dump();
  // }

  // for (auto it = inst_list.begin(); it != inst_list.end(); it++) {
  //   it->dump();
  // }
  printf("Traced Inst Count: 0d%ld\n", instBufferPtr);

  trace_stream->close();
}

uint64_t TraceWriter::compressZSTD(char *dst, uint64_t dst_len, const char *src, uint64_t src_len) {

  ZSTD_CCtx* cctx = ZSTD_createCCtx();
  size_t compressedSize = ZSTD_compressCCtx(cctx, dst, dst_len, src, src_len, 1);
  if (ZSTD_isError(compressedSize)) {
    std::cout << "Compress Error " << ZSTD_getErrorName(compressedSize) << std::endl;
    exit(0);
    return -1;
  }
  std::cout << "Compress Success " << src_len / 1024 / 1024 << "MB -> " << compressedSize / 1024 / 1024 << "MB" << std::endl;
  return compressedSize;
}

void TraceWriter::error_dump() {
  printf("Lastest not finished inst:\n");
  inst.dumpWithID(instBufferPtr);

  printf("TraceWriter::error_dump, recent inst:\n");
  uint64_t num = 0;
  for (auto instIT : inst_list) {
    uint64_t currID = instBufferPtr - inst_list.size() + num++;
    instIT.dumpWithID(currID);
  }

#if defined VERBOSE && defined LogBuffer
  printf("Trace Write Trace:\n");
  for (int i = 0; i < TraceLogEntryNum; i++) {
    printf("%s", trace_log_buf[trace_log_ptr_pop()]);
  }
#endif

  // printf("Call Stack Trace:\n");
  // std::cout << boost::stacktrace::stacktrace();
  // fflush(stdout);
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


char *trace_filename = NULL;
TraceWriter *trace_writer = NULL;

extern "C"  {

void set_trace_writer_file(char *name) {
  trace_filename = name;
}

void init_trace_writer() {
  if (trace_filename == NULL) {
    printf("TraceFile is NULL. please set --tracefile.\n");
    exit(1);
  }
  trace_writer = new TraceWriter(trace_filename);
}

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

void trace_write_arthi_src(uint64_t src0, uint64_t src1) {
  trace_writer->write_arthi_src(src0, src1);
}

void trace_write_branch(uint64_t target, uint8_t branch_type, uint8_t is_taken) {
  trace_writer->write_branch(target, branch_type, is_taken);
}

void trace_write_exception(uint8_t NO, uint64_t target) {
  trace_writer->write_exception(NO, target);
}

void trace_write_interrupt(uint8_t NO, uint64_t target) {
  trace_writer->write_interrupt(NO, target);
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
