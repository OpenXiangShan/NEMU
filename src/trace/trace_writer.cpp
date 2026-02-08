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
// #include <memory/paddr.h>
// #include <boost/stacktrace.hpp>
#include <zstd.h>
#include <vector>

extern "C" {
uint64_t pmem_read(uint64_t addr, int len);
}

TraceWriter::TraceWriter(std::string trace_file_name) {
  std::string tracertl_name = trace_file_name + ".trace.zstd";
  std::string tracertl_page_table_name = trace_file_name + ".pagetable.zstd";

  trace_stream = new std::ofstream(tracertl_name, std::ios_base::out);
  trace_page_table_stream = new std::ofstream(tracertl_page_table_name, std::ios_base::out);
  if ((!trace_stream->is_open())) {
    std::ostringstream oss;
    oss << "[TraceWriter.TraceWriter] Could not open file: " << tracertl_name;
  }
  if ((!trace_page_table_stream->is_open())) {
    std::ostringstream oss;
    oss << "[TraceWriter.TraceWriter] Could not open file: " << trace_page_table_stream;
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

  if (trace_likely(instListSize <= inst_list.size())) {
    inst_list.pop_front();
  }
  inst_list.push_back(inst);

  inst_valid = false;

  if (trace_unlikely(instBufferPtr >= MAX_INSTRUCTION_NUM)) {
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

  if (trace_likely(inst.instr_pc_pa == 0)) {
    inst.instr_pc_pa = pa;
  }
  if (trace_unlikely((inst.instr_pc_pa & 0xfff) != (inst.instr_pc_va & 0xfff))) {
    printf("pc va pa not match: %lx %lx\n", inst.instr_pc_va, inst.instr_pc_pa);
    error_dump();
    exit(1);
  }
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

  if (trace_likely(inst.exu_data.memory_address.pa == 0)) {
    inst.exu_data.memory_address.pa = pa;
  }

  if (trace_unlikely((inst.exu_data.memory_address.pa & 0xfff) != (inst.exu_data.memory_address.va & 0xfff))) {
    printf("mem va pa not match: %lx %lx\n", inst.exu_data.memory_address.va, inst.exu_data.memory_address.pa);
    error_dump();
    exit(1);
  }
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

  if (trace_likely(inst.branch_type == BRANCH_None)) {
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

  printf("Compress Trace file...\n");
  uint64_t compressedSize = compressZSTD(compressedBuffer, sizeof(Instruction) * instBufferPtr, (char *)instBuffer, instBufferSize);

  trace_stream->write(compressedBuffer, compressedSize);
  trace_stream->close();

  // for (uint64_t i = 0; i < instBufferPtr; i++) {
  //   instBuffer[i].dump();
  // }
  std::map<uint64_t, uint64_t> pc_num_map;
  for (uint64_t i = 0; i < instBufferPtr; i++) {
    uint64_t pc = instBuffer[i].instr_pc_va;
    if (pc_num_map.find(pc) == pc_num_map.end()) {
      pc_num_map[pc] = 1;
    } else {
      pc_num_map[pc]++;
    }
  }
  printf("Unique PC Num: %ld\n", pc_num_map.size());
  printf("Total Inst Num: %ld\n", instBufferPtr);
  for (auto it = pc_num_map.begin(); it != pc_num_map.end(); it++) {
    // if (it->second > 100000) {
    printf("  PC: 0x%lx Num: %ld\n", it->first, it->second);
    // }
  }


  printf("Traced Inst Count: 0d%ld\n", instBufferPtr);

#ifdef TRACE_DUMP_PAGE_TABLE
  chooseSatp();
  if (satp == 0) {
    printf("Satp not setted\n");
    exit(1);
  }

  readPageTable();
  setFreePageFrameAddr();
  modifyPageTable();
  dumpPageTable();

#endif
}

void TraceWriter::chooseSatp() {
  printf("Choose Satp...\n");
  satp = 0;
  uint64_t num = 0;
  for (auto it = satp_list.begin(); it != satp_list.end(); it++) {
    printf("   Satp: 0x%lx Num:%ld\n", it->first, it->second);
    if (it->second > num) {
      num = it->second;
      satp = it->first;
    }
  }
  printf("  Final Satp: 0x%lx number:%ld\n", satp, num);
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

void TraceWriter::dfs_dump_entry(uint64_t base_paddr, int level) {
  if (base_paddr < 0x80000000L) {
    printf("Error: base_paddr: 0x%lx less than 0x80000000L. Level:%d\n", base_paddr, level);
    exit(1);
  }
  if (level <  0) return;

  // printf("level: %d, base_paddr: 0x%lx\n", level, base_paddr);
  for (size_t idx = 0; idx < TRACE_PAGE_ENTRY_NUM; idx++) {
    uint64_t paddr = base_paddr + idx * TRACE_PAGE_ENTRY_SIZE;
    uint64_t pte_val = pmem_read(paddr, 8);
    TracePTE pte;
    pte.val = pte_val;
    // printf("  Idx:%ld, paddr: 0x%lx, pte: 0x%lx, ppn: 0x%lx level:%d base_paddr:%lx\n", idx, paddr, pte_val, pte.ppn, level, base_paddr);
    // fflush(stdout);
    if (pte.v == 0) {
      // printf("    invalid pte: 0x%lx v:%d\n", pte_val, pte.v);
      continue;
    }
    if ((level == 0) && !(pte.r || pte.w || pte.x)) {
      printf("    Wrong PTE %lx(vpn:%lx) level: %d. invalid perm\n", pte.val, pte.ppn, level);
      fflush(stdout);
      exit(1);
    }

    TracePageEntry entry;
    entry.paddr = paddr;
    entry.pte = pte_val;
    entry.level = (uint64_t)level;
    pageTable.push_back(entry);
    pageTableMap[paddr] = pte_val;

    bool is_leaf = (level == 0) || (pte.v && (pte.r || pte.w || pte.x));
    // printf("    pageTableMap[0x%lx] = 0x%lx level %d leaf %d\n", paddr, pte_val, level, is_leaf);
    // fflush(stdout);
    if (!is_leaf) {
      uint64_t next_base_paddr = pte.ppn << TRACE_PAGE_SHIFT;
      if (pte.ppn == 0) {
        printf("level:%d pte.val:%lx pte.ppn:%lx\n", level-1, pte.val, pte.ppn);
        exit(1);
      }
      dfs_dump_entry(next_base_paddr, level - 1);
    }
  }
}

uint64_t TraceWriter::trans(uint64_t vpn, bool &hit, bool verbose) {
  hit = false;

  uint64_t pgBase = (satp & TRACE_SATP64_PPN) << TRACE_PAGE_SHIFT;
  int level = 2;
  if (verbose) {
    printf("vpn: %lx, pgBase: %lx initLevel:%d\n", vpn, pgBase, level);
  }
  for (; level >= 0; level--) {
    uint64_t pteAddr = GetPteAddr(vpn, level, pgBase);
    uint64_t pteVal = pageTableMap[pteAddr];
    TracePTE pte;
    pte.val = pteVal;

    if( verbose) {
      printf("  level: %d, pteAddr: %lx, pteVal: %lx(%lx)\n", level, pteAddr, pteVal, pte.ppn);
    }

    if (!pte.v) {
      if (verbose) {
        printf("Error: vpn: %lx, level: %d, pte: %lx, pteAddr:%lx\n", vpn, level, pteVal, pteAddr);
      }
      break;
      // exit(1);
    } else if (!(pte.r || pte.w || pte.x)) {
      if (verbose) {
        printf("    non-leaf pte, go to next level\n");
      }
      pgBase = pte.ppn << TRACE_PAGE_SHIFT;
    } else {
      hit = true;
      uint64_t ppn = 0;
      switch (level) {
        case 0: ppn = pte.ppn; break;
        case 1: ppn = pte.ppn | (vpn & 0x1ff); break;
        case 2: ppn = pte.ppn | (vpn & 0x3ffff); break;
        default: printf("Error: level: %d\n", level); break;
      }
      if (verbose) {
        printf("    leaf pte, return ppn: %lx\n", ppn);
      }
      return ppn;
    }
  }
  if (verbose) {
    printf("  Should not be here: no entry match for vpn: %lx\n", vpn);
  }
  return pgBase >> 12;
}

void TraceWriter::setFreePageFrameAddr() {
  uint64_t freePageFrameAddr = 0;
  for (auto it = pageTable.begin(); it != pageTable.end(); it++) {
    freePageFrameAddr = (freePageFrameAddr > it->paddr) ? freePageFrameAddr : it->paddr;
  }
  freePageFramePPN = (freePageFrameAddr >> 12) + 1;
}

void TraceWriter::modifyPageTable() {
  // test
  uint64_t tested_trans_num = 0;
  uint64_t unmatched_trans_num = 0;

  // std::map<TracePageTrans, TracePageTransTo> unmatch_map;
  // std::map<TracePageTrans, TracePageTransTo> match_map;
  std::map<uint64_t, TracePageTransTo> unmatch_map;
  std::map<uint64_t, TracePageTransTo> match_map;

  // uint64_t count_num = 0;
  for (size_t i = 0; i < instBufferPtr; i++) {
    if (instBuffer[i].exception == 0) {
      uint64_t pc_va = instBuffer[i].instr_pc_va;
      uint64_t pc_pa = instBuffer[i].instr_pc_pa;

      bool unmatched = false;
      if (pc_va != pc_pa && pc_pa != 0) {
        tested_trans_num++;
        bool verbose = tested_trans_num == 37;
        // bool verbose = (tested_trans_num > 10000 && tested_trans_num < 10010);
        // bool verbose = false;
        // bool verbose = true;
        bool hit;
        uint64_t pc_vpn = (pc_va >> 12);
        uint64_t pc_ppn = (pc_pa >> 12);
        if (verbose) {
          printf("Tested trans %ld: pc_vpn: 0x%lx, pc_ppn: 0x%lx\n", tested_trans_num, pc_vpn, pc_ppn);
        }
        uint64_t dumpPage_ppn_pc = trans(pc_vpn, hit, verbose);
        if (!hit || dumpPage_ppn_pc != (pc_ppn)) {
          // if (count_num++ < 10) {
          //   printf("Error: %ld trans error. pc_vpn: 0x%lx, pc_ppn: 0x%lx, hit: %d, dumpPage_ppn_pc: 0x%lx\n", tested_trans_num, pc_vpn, pc_ppn, hit, dumpPage_ppn_pc);
          //   instBuffer[i].dump();
          // }
          unmatched = true;
          // verbose = true;
          if (unmatch_map.find(pc_vpn) == unmatch_map.end()) {
            unmatch_map[pc_vpn] = {dumpPage_ppn_pc, pc_ppn, 1};
          } else {
            unmatch_map[pc_vpn].num++;
          }
        } else {
          if (match_map.find(pc_vpn) == match_map.end()) {
            match_map[pc_vpn] = {dumpPage_ppn_pc, pc_ppn, 1};
          } else {
            match_map[pc_vpn].num++;
          }
        }

        if (instBuffer[i].memory_type != MEM_TYPE_None) {
          uint64_t mem_vpn = instBuffer[i].exu_data.memory_address.va >> 12;
          uint64_t mem_ppn = instBuffer[i].exu_data.memory_address.pa >> 12;
          bool hit;
          if (verbose) {
            printf("Tested trans %ld: mem_vpn: 0x%lx, mem_ppn: 0x%lx\n", tested_trans_num, mem_vpn, mem_ppn);
          }
          uint64_t dumpPage_ppn_mem = trans(mem_vpn, hit, verbose);
          if (!hit || dumpPage_ppn_mem != mem_ppn) {
            // if (count_num++ < 10) {
            //   printf("Error: %ld trans error. vpn: 0x%lx, ppn: 0x%lx, hit:%d, dumpPage_ppn: 0x%lx\n", tested_trans_num, mem_vpn, mem_ppn, hit, dumpPage_ppn_mem);
            //   instBuffer[i].dump();
            // }
            unmatched = true;
            if (unmatch_map.find(mem_vpn) == unmatch_map.end()) {
              unmatch_map[mem_vpn] = {dumpPage_ppn_mem, mem_ppn, 1};
            } else {
              unmatch_map[mem_vpn].num++;
            }
          } else {
            if (match_map.find(mem_vpn) == match_map.end()) {
              match_map[mem_vpn] = {dumpPage_ppn_mem, mem_ppn, 1};
            } else {
              match_map[mem_vpn].num++;
            }
          }
        }
      }

      if (unmatched) {
        unmatched_trans_num++;
      }
    }
  }
  printf("Trans tested num: %ld unmatched %ld\n", tested_trans_num, unmatched_trans_num);

  for (auto it = unmatch_map.begin(); it != unmatch_map.end(); it++) {
    uint64_t vpn = it->first;
    uint64_t pt_ppn = it->second.pt_ppn;
    uint64_t wantted_ppn = it->second.wantted_ppn;
    uint64_t unmatch_num = it->second.num;
    uint64_t match_num = 0;
    if (match_map.find(vpn) != match_map.end()) {
      match_num = match_map[vpn].num;
    }
    printf("Unmatched trans: vpn: 0x%lx, um_ppn: 0x%lx, m_ppn:0x%lx, unmatched num: %ld matched num:%ld \n", vpn, pt_ppn, wantted_ppn, unmatch_num, match_num);
  }

  for (auto it = unmatch_map.begin(); it != unmatch_map.end(); it++) {
    uint64_t vpn = it->first;
    // if (vpn >> (6*4) == 0xffffffeL) {
      uint64_t pt_ppn = it->second.pt_ppn;
      uint64_t wantted_ppn = it->second.wantted_ppn;
      uint64_t num = it->second.num;

      if ((match_map.find(vpn) == match_map.end()) ||
          (num > match_map[vpn].num)) {
        if (!overridePageMap(vpn, wantted_ppn, false)) {
          printf("Failed to override page map\n");
          printf("vpn: 0x%lx, ppn: 0x%lx\n", vpn, wantted_ppn);
          exit(1);
        } else {
          printf("Override vpn 0x%lx: ppn 0x%lx -> 0x%lx\n", vpn, pt_ppn, wantted_ppn);
        }
      }
  }
}

bool TraceWriter::overridePageMap(uint64_t vpn, uint64_t ppn, bool verbose) {
  if (verbose) {
    printf("OverridePageMap: vpn: 0x%lx, ppn: 0x%lx\n", vpn, ppn);
  }
  uint64_t pgBase = (satp & TRACE_SATP64_PPN) << TRACE_PAGE_SHIFT;
  int level = 2;

  // get free PageFrame
  for (; level >= 0; level--) {
    uint64_t pteAddr = GetPteAddr(vpn, level, pgBase);
    uint64_t pteVal = pageTableMap[pteAddr];
    TracePTE pte;
    pte.val = pteVal;

    if (verbose) {
      printf("  level: %d, pteAddr: %lx, pteVal: %lx(%lx)\n", level, pteAddr, pteVal, pte.ppn);
    }

    if (pte.v && !(pte.r || pte.w || pte.x)) {
      if (verbose) printf("  level %d non-leaf pte, go to next level\n", level);
    } else {
      TracePTE newPte = (level == 0) ? genLeafPte(ppn) : genNonLeafPte(freePageFramePPN++);
      if (verbose) printf("  level %d leaf pte override: %lx -> %lx\n", level, pte.val, newPte.val);
      pte.val = newPte.val;
      if (level == 0) return true;
    }
    pgBase = pte.ppn << TRACE_PAGE_SHIFT;
  }

  return false;
}

inline TracePTE TraceWriter::genLeafPte(uint64_t ppn) {
  TracePTE pte;
  pte.val = 0;
  pte.v = 1;
  pte.r = 1;
  pte.w = 1;
  pte.x = 1;
  pte.u = 1;
  pte.a = 1;
  pte.d = 1;
  pte.ppn = ppn;
  return pte;
}

inline TracePTE TraceWriter::genNonLeafPte(uint64_t ppn) {
  TracePTE pte;
  pte.val = 0;
  pte.v = 1;
  pte.ppn = ppn;
  return pte;
}

void TraceWriter::readPageTable() {
  uint64_t base_paddr = (satp & TRACE_SATP64_PPN) << TRACE_PAGE_SHIFT;
  printf("readPageTable: satp:%lx base_paddr: 0x%lx\n", satp, base_paddr);
  fflush(stdout);
  dfs_dump_entry(base_paddr, 2);

  // uint64_t count = 0;
  // for (auto &entry : pageTableMap) {
  //   printf("Page Table %ld: paddr %lx pte %lx\n", count++, entry.first, entry.second);
  // }

  if (pageTableMap.size() < 100) {
    printf("Error: Page Table size: %ld < 100. Maybe wrong. Check it\n", pageTableMap.size());
    fflush(stdout);
    exit(1);
  }
}

void TraceWriter::dumpPageTable() {
  uint64_t pageTableNum = pageTable.size() + 1;
  uint64_t pageTableMemSize = pageTableNum * sizeof(TracePageEntry);
  TracePageEntry *pageTableBuffer = (TracePageEntry *)malloc(pageTableMemSize);
  // use the first entry to store the satp
  pageTableBuffer[0].paddr = pageTableNum;
  pageTableBuffer[0].pte = satp;
  pageTableBuffer[0].level = 0;
  for (size_t i = 0; i < pageTable.size(); i++) {
    pageTableBuffer[i + 1] = pageTable[i];
  }

  printf("Compress Page Table...\n");
  char *pageTableCompressedBuffer = (char *)malloc(pageTableMemSize);
  uint64_t compressedSize = compressZSTD(
    (char *)pageTableCompressedBuffer, pageTableMemSize,
    (char *)pageTableBuffer, pageTableMemSize);

  trace_page_table_stream->write(pageTableCompressedBuffer, compressedSize);

  trace_page_table_stream->close();

  free(pageTableBuffer);
  free(pageTableCompressedBuffer);
}

inline void TraceWriter::insertSatp(uint64_t outer_satp) {
  // printf("Set satp: from %lx to %lx\n", satp, outer_satp);
  // satp = outer_satp;
  if (satp_list.find(outer_satp) == satp_list.end()) {
    satp_list[outer_satp] = 1;
  } else {
    satp_list[outer_satp]++;
  }
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
  if (trace_unlikely(trace_filename == NULL)) {
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


void trace_write_setSatp(uint64_t satp) {
  trace_writer->insertSatp(satp);
}

void trace_inst_over() {
  trace_writer->inst_over();
}

void trace_end() {
  printf("Trace end for execution end.\n");
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
