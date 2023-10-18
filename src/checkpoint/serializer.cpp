/***************************************************************************************
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

//
// Created by zyy on 2020/11/16.
//

#include <checkpoint/path_manager.h>
#include <profiling/profiling_control.h>
#include <checkpoint/serializer.h>
#include <checkpoint/cpt_env.h>

#include "../isa/riscv64/local-include/csr.h"

#include <isa.h>
#include <common.h>

#include <cinttypes>
#include <iostream>
#include <zlib.h>
#include <limits>
#include <string>

#include <fstream>
#include <gcpt_restore/src/restore_rom_addr.h>

using std::cout;
using std::cerr;
using std::endl;
using std::fstream;
using std::string;
using std::to_string;
using std::numeric_limits;

Serializer::Serializer() :
    IntRegStartAddr(INT_REG_CPT_ADDR - BOOT_CODE),
    FloatRegStartAddr(FLOAT_REG_CPT_ADDR - BOOT_CODE),
    CSRStartAddr(CSR_CPT_ADDR - BOOT_CODE),
    PCAddr(PC_CPT_ADDR - BOOT_CODE),
    CptFlagAddr(BOOT_FLAGS - BOOT_CODE)
{

}

extern "C" {
uint8_t *get_pmem();
word_t paddr_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr);
uint8_t* guest_to_host(paddr_t paddr);
#include <debug.h>
extern bool log_enable();
extern void log_flush();
extern unsigned long MEMORY_SIZE;
}

void Serializer::serializePMem(uint64_t inst_count) {
  // We must dump registers before memory to store them in the Generic Arch CPT
  assert(regDumped);
  const size_t PMEM_SIZE = MEMORY_SIZE;
  uint8_t *pmem = get_pmem();

  assert(restorer);
  FILE *fp = fopen(restorer, "rb");
  if (!fp) {
    xpanic("Cannot open restorer %s\n", restorer);
  }
  uint32_t restorer_size = 0x400;
  fseek(fp, 0, SEEK_SET);
  assert(restorer_size == fread(pmem, 1, restorer_size, fp));
  fclose(fp);
  Log("Put gcpt restorer %s to start of pmem", restorer);

  string filepath;
  if (checkpoint_state == SimpointCheckpointing) {
      filepath = pathManager.getOutputPath() + "_" + \
                        to_string(simpoint2Weights.begin()->first) + "_" + \
                        to_string(simpoint2Weights.begin()->second) + "_.gz";
  } else {
      filepath = pathManager.getOutputPath() + "_" + \
                        to_string(inst_count) + "_.gz";
  }

  gzFile compressed_mem = gzopen(filepath.c_str(), "wb");
  if (compressed_mem == nullptr) {
    cerr << "Failed to open " << filepath << endl;
    xpanic("Can't open physical memory checkpoint file!\n");
  } else {
    cout << "Opening " << filepath << " as checkpoint output file" << endl;
  }

  uint64_t pass_size = 0;

  for (uint64_t written = 0; written < PMEM_SIZE; written += pass_size) {
    pass_size = numeric_limits<int>::max() < ((int64_t) PMEM_SIZE - (int64_t) written) ?
                numeric_limits<int>::max() : ((int64_t) PMEM_SIZE - (int64_t) written);

    if (gzwrite(compressed_mem, pmem + written, (uint32_t) pass_size) != (int) pass_size) {
      xpanic("Write failed on physical memory checkpoint file\n");
    }
    Log("Written 0x%lx bytes\n", pass_size);
  }

  if (gzclose(compressed_mem)){
    xpanic("Close failed on physical memory checkpoint file\n");
  }
  Log("Checkpoint done!\n");
  regDumped = false;
}

extern void csr_writeback();

void Serializer::serializeRegs() {
  auto *intRegCpt = (uint64_t *) (get_pmem() + IntRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(intRegCpt + i) = cpu.gpr[i]._64;
  }
  Log("Writing int registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)",
      INT_REG_CPT_ADDR, INT_REG_CPT_ADDR + 32 * 8,
      IntRegStartAddr, IntRegStartAddr + 32 * 8
      );

#ifndef CONFIG_FPU_NONE
  auto *floatRegCpt = (uint64_t *) (get_pmem() + FloatRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(floatRegCpt + i) = cpu.fpr[i]._64;
  }
  Log("Writing float registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)",
      FLOAT_REG_CPT_ADDR, FLOAT_REG_CPT_ADDR + 32 * 8,
      FloatRegStartAddr, FloatRegStartAddr + 32 * 8
      );
#endif // CONFIG_FPU_NONE

  auto *pc = (uint64_t *) (get_pmem() + PCAddr);
  *pc = cpu.pc;
  Log("Writing PC: 0x%lx at addr 0x%x", cpu.pc, PC_CPT_ADDR);


//  csr_writeback();
  auto *csrCpt = (uint64_t *) (get_pmem() + CSRStartAddr);
//  Log("csrCpt: %p\n",csrCpt);
//  Log("Mstatus: 0x%x", mstatus->val);
//  Log("CSR array mstatus: 0x%x", csr_array[0x300]);
  for (unsigned i = 0; i < 4096; i++) {
    rtlreg_t val = csr_array[i];

    if ((void *)mip == (void *)&csr_array[i]) {
      mip_t mip_tmp = *mip;
      if (mip_tmp.mtip) {
        mip_tmp.mtip = 0;
      }
//      Log("Saving mip: 0x%x", mip_tmp.val);
      val = mip_tmp.val;
    }

    *(csrCpt + i) = val;

    if (csr_array[i] != 0) {
      Log("CSR 0x%x: 0x%lx", i, *(csrCpt + i));
    }
  }
  Log("Writing CSR to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)",
      CSR_CPT_ADDR, CSR_CPT_ADDR + 4096 * 8,
      CSRStartAddr, CSRStartAddr + 4096 * 8
      );


  auto *flag = (uint64_t *) (get_pmem() + CptFlagAddr);
  *flag = CPT_MAGIC_BUMBER;
  Log("Touching Flag: 0x%x at addr 0x%x", CPT_MAGIC_BUMBER, BOOT_FLAGS);

  auto *mode_flag = (uint64_t *) (get_pmem() + CptFlagAddr + 8);
  *mode_flag = cpu.mode;
  Log("Record mode flag: 0x%lx at addr 0x%x", cpu.mode, BOOT_FLAGS+8);

  auto *mtime = (uint64_t *) (get_pmem() + CptFlagAddr + 16);
  extern word_t paddr_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr);
  *mtime = ::paddr_read(CLINT_MMIO+0xBFF8, 8, MEM_TYPE_READ, MODE_M, CLINT_MMIO+0xBFF8);
  Log("Record time: 0x%lx at addr 0x%x", cpu.mode, BOOT_FLAGS+16);

  auto *mtime_cmp = (uint64_t *) (get_pmem() + CptFlagAddr + 24);
  *mtime_cmp = ::paddr_read(CLINT_MMIO+0x4000, 8, MEM_TYPE_READ, MODE_M, CLINT_MMIO+0x4000);
  Log("Record time: 0x%lx at addr 0x%x", cpu.mode, BOOT_FLAGS+24);

  regDumped = true;
}

void Serializer::serialize(uint64_t inst_count) {
//  isa_reg_display();
  serializeRegs();
  serializePMem(inst_count);

//  isa_reg_display();
}

void Serializer::init() {
  if  (checkpoint_state == SimpointCheckpointing) {
    assert(checkpoint_interval);
    intervalSize = checkpoint_interval;
    Log("Taking simpoint checkpionts with profiling interval %lu",
        checkpoint_interval);

    auto simpoints_file = fstream(pathManager.getSimpointPath() + "simpoints0");
    auto weights_file = fstream(pathManager.getSimpointPath() + "weights0");
    assert(!simpoints_file.bad());
    assert(!weights_file.bad());

    uint64_t simpoint_location, simpoint_id, weight_id;
    double weight;

    while (simpoints_file >> simpoint_location >> simpoint_id) {
      assert(weights_file >> weight >> weight_id);
      assert(weight_id == simpoint_id);
      simpoint2Weights[simpoint_location] = weight;

      Log("Simpoint %lu: @ %lu, weight: %f", simpoint_id, simpoint_location, weight);
    }

  } else if (checkpoint_state==UniformCheckpointing||checkpoint_state==ManualUniformCheckpointing) {
    assert(checkpoint_interval);
    intervalSize = checkpoint_interval;
    Log("Taking uniform checkpionts with interval %lu", checkpoint_interval);
    nextUniformPoint = intervalSize;
  }
  pathManager.setCheckpointingOutputDir();
}

bool Serializer::instrsCouldTakeCpt(uint64_t num_insts) {
  switch (checkpoint_state) {
    case SimpointCheckpointing:
      if (simpoint2Weights.empty()) {
        break;
      }else{
        uint64_t next_point = simpoint2Weights.begin()->first * intervalSize + 100000;
        if (num_insts >= next_point) {
          Log("Should take cpt now: %lu", num_insts);
          return true;
        } else if (num_insts % intervalSize == 0) {
          Log("First cpt @ %lu, now: %lu",
          next_point, num_insts);
          break;
        }else{
          break;
        }
      }
    case ManualOneShotCheckpointing:
      return true;
    case ManualUniformCheckpointing:
    case UniformCheckpointing:
      if (num_insts >= nextUniformPoint) {
          Log("Should take cpt now: %lu", num_insts);
          return true;
      }
      break;
    case NoCheckpoint:
      break;
    default:
      break;
  }
  return false;
}

void Serializer::notify_taken(uint64_t i) {
  Log("Taking checkpoint @ instruction count %lu", i);
  if (checkpoint_state == SimpointCheckpointing) {
    Log("simpoint2Weights size: %ld", simpoint2Weights.size());

    if (!simpoint2Weights.empty()) {
      simpoint2Weights.erase(simpoint2Weights.begin());
    }

    if (!simpoint2Weights.empty()) {
        pathManager.setCheckpointingOutputDir();
    } 

  } else if (checkpoint_state==ManualUniformCheckpointing||checkpoint_state==UniformCheckpointing) {
    nextUniformPoint += intervalSize;
    pathManager.setCheckpointingOutputDir();
  }
}

Serializer serializer;
uint64_t Serializer::next_index(){
  uint64_t index=0;
  if (checkpoint_state==SimpointCheckpointing&&!serializer.simpoint2Weights.empty()) {
    index=serializer.simpoint2Weights.begin()->first;
  }else if(checkpoint_state==UniformCheckpointing||checkpoint_state==ManualUniformCheckpointing){
    index=nextUniformPoint;
  }
  Log("set next index %ld",index);
  return index;
}


extern "C" {

void init_serializer() {
  serializer.init();
}

bool try_take_cpt(uint64_t icount) {
  if (serializer.instrsCouldTakeCpt(icount)) {
    serializer.serialize(icount);
    serializer.notify_taken(icount);
    Log("return true");
    return true;
  }
  return false;
}

void serialize_reg_to_mem() {
  serializer.serializeRegs();
}

}
