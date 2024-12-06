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

#include <checkpoint/cpt_env.h>
#include <checkpoint/path_manager.h>
#include <checkpoint/serializer.h>
#include <profiling/profiling_control.h>

#include "../isa/riscv64/local-include/csr.h"

#include <common.h>
#include <isa.h>

#include <iostream>
#include <limits>
#include <stdint.h>
#include <string>
#include <zlib.h>

#include <fcntl.h>
#include <fstream>
#include <gcpt_restore/src/restore_rom_addr.h>
#include <zstd.h>

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::numeric_limits;
using std::string;
using std::to_string;

extern "C" {
uint8_t *get_pmem();
word_t paddr_read(paddr_t addr, int len, int type, int trap_type, int mode, vaddr_t vaddr);
uint8_t *guest_to_host(paddr_t paddr);
#include <debug.h>
extern void log_buffer_flush();
extern void log_file_flush();
extern unsigned long MEMORY_SIZE;
extern uint8_t* get_flash_base();
void encode_cpt_header(checkpoint_header *cpt_header, single_core_rvgc_rvv_rvh_memlayout *cpt_percpu_layout);
extern uint64_t get_flash_size();
#include <debug.h>
#include <checkpoint/fill_protobuf.h>
#include <checkpoint/checkpoint.pb.h>
}

#ifdef CONFIG_MEM_COMPRESS
void Serializer::serializePMem(uint64_t inst_count, bool write_to_flash, uint8_t *pmem_addr, uint8_t *gcpt_mmio_addr) {
  // We must dump registers before memory to store them in the Generic Arch CPT
  assert(regDumped);
  const size_t PMEM_SIZE = MEMORY_SIZE;
  const size_t GCPT_MMIO_SIZE = get_flash_size();
  Log("Host physical address: %p size: %lx", pmem_addr, PMEM_SIZE);
  Log("Host gcpt address: %p size: %lx", gcpt_mmio_addr, GCPT_MMIO_SIZE);

  string base_file_path;
  string flash_file_path;
  string memory_file_path;

  if (checkpoint_state == SimpointCheckpointing) {
    base_file_path = pathManager.getOutputPath() + "_" + to_string(simpoint2Weights.begin()->first) + "_" +
               to_string(simpoint2Weights.begin()->second);
  } else {
    base_file_path = pathManager.getOutputPath() + "_" + to_string(inst_count);
  }

  if (compress_file_format == GZ_FORMAT) {
    Log("Using GZ format generate checkpoint");
    flash_file_path += "_flash_.gz";
    memory_file_path += "_memory_.gz";
    gzFile flash_compressed_mem = gzopen(flash_file_path.c_str(), "wb");
    gzFile memory_compressed_mem = gzopen(memory_file_path.c_str(), "wb");
    if (flash_compressed_mem == nullptr) {
      cerr << "Failed to open " << flash_file_path << endl;
      xpanic("Can't open physical memory checkpoint file!\n");
    } else {
      cout << "Opening " << flash_file_path << " as checkpoint output file" << endl;
    }

    if (memory_compressed_mem == nullptr) {
      cerr << "Failed to open " << memory_file_path << endl;
      xpanic("Can't open physical memory checkpoint file!\n");
    } else {
      cout << "Opening " << memory_file_path << " as checkpoint output file" << endl;
    }

    uint64_t pass_size = 0;
    if (write_to_flash) {
      for (uint64_t gcpt_written = 0; gcpt_written < GCPT_MMIO_SIZE; gcpt_written += pass_size) {
        pass_size = numeric_limits<int>::max() < ((int64_t)GCPT_MMIO_SIZE - (int64_t)gcpt_written)
                      ? numeric_limits<int>::max()
                      : ((int64_t)GCPT_MMIO_SIZE - (int64_t)gcpt_written);
        if (gzwrite(flash_compressed_mem, gcpt_mmio_addr + gcpt_written, (uint32_t)pass_size) != (int)pass_size) {
          xpanic("Write failed on physical memory checkpoint file\n");
        }
        Log("Written 0x%lx bytes\n", pass_size);
      }

      pass_size = 0;
    }

    for (uint64_t written = 0; written < PMEM_SIZE; written += pass_size) {
      pass_size = numeric_limits<int>::max() < ((int64_t)PMEM_SIZE - (int64_t)written)
                    ? numeric_limits<int>::max()
                    : ((int64_t)PMEM_SIZE - (int64_t)written);

      if (gzwrite(memory_compressed_mem, pmem_addr + written, (uint32_t)pass_size) != (int)pass_size) {
        xpanic("Write failed on physical memory checkpoint file\n");
      }
      Log("Written 0x%lx bytes\n", pass_size);
    }

    if (gzclose(flash_compressed_mem)) {
      xpanic("Close failed on physical checkpoint file\n");
    }
    if (gzclose(memory_compressed_mem)) {
      xpanic("Close failed on physical checkpoint file\n");
    }

  } else if (compress_file_format == ZSTD_FORMAT) {
    Log("Using ZSTD format generate checkpoint");

    flash_file_path += "_flash_.zstd";
    memory_file_path += "_memory_.zstd";
 
    Log("Opening %s as checkpoint output file", flash_file_path.c_str());
    Log("Opening %s as memory output file", memory_file_path.c_str());

    // zstd compress
    size_t flash_size = GCPT_MMIO_SIZE;
    size_t memory_size = PMEM_SIZE;

    size_t const flash_compress_buffer_size = ZSTD_compressBound(flash_size);
    size_t const memory_compress_buffer_size = ZSTD_compressBound(memory_size);
    uint8_t *const flash_compress_buffer = (uint8_t*)malloc(flash_compress_buffer_size);
    uint8_t *const memory_compress_buffer = (uint8_t*)malloc(memory_compress_buffer_size);
    assert(flash_compress_buffer);
    assert(memory_compress_buffer);

    // compress gcpt device memory
    size_t flash_compress_size = 0;
    if (write_to_flash) {
      flash_compress_size = ZSTD_compress(flash_compress_buffer, flash_compress_buffer_size, gcpt_mmio_addr, GCPT_MMIO_SIZE, 1);
      assert(flash_compress_size <= flash_compress_buffer_size && flash_compress_size != 0);
      Log("compress gcpt success, compress size %ld", flash_compress_size);
    }

    size_t memory_compress_size = ZSTD_compress(memory_compress_buffer, memory_compress_buffer_size, pmem_addr, PMEM_SIZE, 1);
    assert(memory_compress_size <= memory_compress_buffer_size && memory_compress_size != 0);
    Log("pmem compress success, compress size %ld", memory_compress_size);

    FILE *flash_compress_file = fopen(flash_file_path.c_str(), "wb");
    size_t flash_fw_size = fwrite(flash_compress_buffer, 1, flash_compress_size, flash_compress_file);

    FILE *memory_compress_file = fopen(memory_file_path.c_str(), "wb");
    size_t memory_fw_size = fwrite(memory_compress_buffer, 1, memory_compress_size, memory_compress_file);

    if (flash_fw_size != flash_compress_size || flash_fw_size != memory_fw_size) {
      fclose(flash_compress_file);
      fclose(memory_compress_file);
      free(flash_compress_buffer);
      free(memory_compress_buffer);
      xpanic("file write error: %s : %s, %s : %s \n", flash_file_path.c_str(), strerror(errno), memory_file_path.c_str(), strerror(errno));
    }

    if (fclose(flash_compress_file)) {
      free(flash_compress_buffer);
      xpanic("file close error: %s : %s \n", base_file_path.c_str(), strerror(errno));
    }

    if (fclose(memory_compress_file)) {
      free(memory_compress_buffer);
      xpanic("file close error: %s : %s \n", base_file_path.c_str(), strerror(errno));
    }

    free(flash_compress_buffer);
    free(memory_compress_buffer);
  } else {
    xpanic("You need to specify the compress file format using: --checkpoint-format\n");
  }

  Log("Checkpoint done!\n");
  regDumped = false;
}
#else
void Serializer::serializePMem(uint64_t inst_count) {}
#endif

#ifdef CONFIG_MEM_COMPRESS
extern void csr_writeback();

void Serializer::serializeRegs(bool write_to_flash, uint8_t *serialize_base_addr, single_core_rvgc_rvv_rvh_memlayout *cpt_percpu_layout) {
  // int reg
  uint64_t buffer_start = (uint64_t)serialize_base_addr + cpt_percpu_layout->int_reg_cpt_addr;
  auto *intRegCpt = (uint64_t *)(buffer_start);
  for (unsigned i = 0; i < 32; i++) {
    *(intRegCpt + i) = cpu.gpr[i]._64;
  }
  Log("Writing int registers to checkpoint memory @host_paddr: [0x%lx, 0x%lx) @mem_layout_addr: [0x%lx, 0x%lx)",
      buffer_start, buffer_start + 32 * 8, cpt_percpu_layout->int_reg_cpt_addr,
      cpt_percpu_layout->int_reg_cpt_addr + 32 * 8);

  // fp reg
#ifndef CONFIG_FPU_NONE
  buffer_start = (uint64_t)serialize_base_addr + cpt_percpu_layout->float_reg_cpt_addr;
  auto *floatRegCpt = (uint64_t *)(buffer_start);
  for (unsigned i = 0; i < 32; i++) {
    *(floatRegCpt + i) = cpu.fpr[i]._64;
  }
  Log("Writing float registers to checkpoint memory @host_paddr: [0x%lx, 0x%lx) @mem_layout_addr: [0x%lx, 0x%lx)",
      buffer_start, buffer_start + 32 * 8, cpt_percpu_layout->float_reg_cpt_addr,
      cpt_percpu_layout->float_reg_cpt_addr + 32 * 8);
#endif  // CONFIG_FPU_NONE

  // rvv reg
#ifdef CONFIG_RVV
  buffer_start = (uint64_t)serialize_base_addr + cpt_percpu_layout->vector_reg_cpt_addr;
  auto *vectorRegCpt = (uint64_t *)(buffer_start);
  for (unsigned i = 0; i < 32; i++) {
    for (unsigned j = 0; j < VENUM64; j++) {
      *(vectorRegCpt + (i * VENUM64) + j) = cpu.vr[i]._64[j];
    }
  }
  Log("Writing vector registers to checkpoint memory @host_paddr [0x%lx, 0x%lx) @mem_layout_addr: [0x%lx, 0x%lx)",
      buffer_start, buffer_start + 32 * 8 * VENUM64, cpt_percpu_layout->vector_reg_cpt_addr,
      cpt_percpu_layout->vector_reg_cpt_addr + 32 * 8 * VENUM64);
#endif  // CONFIG_RVV

  // pc
  buffer_start = (uint64_t)serialize_base_addr + cpt_percpu_layout->pc_cpt_addr;
  auto *pc = (uint64_t *)(buffer_start);
  *pc = cpu.pc;
  Log("Writing PC: 0x%lx to checkpoint memory @host_paddr [0x%lx, 0x%lx) @mem_layout_addr: [0x%lx, 0x%lx)", cpu.pc,
      buffer_start, buffer_start + 8, cpt_percpu_layout->pc_cpt_addr, cpt_percpu_layout->pc_cpt_addr + 8);

  // csr reg
  // donot need reset mip.x
  buffer_start = (uint64_t)serialize_base_addr + cpt_percpu_layout->csr_reg_cpt_addr;
  auto *csrCpt = (uint64_t *)(buffer_start);
  for (unsigned i = 0; i < 4096; i++) {
    *(csrCpt + i) = csr_array[i];
    if (csr_array[i] != 0) {
      Log("CSR id: 0x%x, value: 0x%lx", i, *(csrCpt + i));
    }
  }

  // priv mode
  buffer_start = (uint64_t)serialize_base_addr + cpt_percpu_layout->mode_cpt_addr;
  auto *mode_flag = (uint64_t *)(buffer_start);
  *mode_flag = cpu.mode;
  Log("Record mode flag: 0x%lx to checkpoint memory @host_paddr [0x%lx, 0x%lx) @mem_layout_addr: [0x%lx, 0x%lx)",
      cpu.mode, buffer_start, buffer_start + 8, cpt_percpu_layout->mode_cpt_addr,
      cpt_percpu_layout->mode_cpt_addr + 8);

  if (*mode_flag == PRV_M) {
    Log("Generate this checkpoint from M mode !!!!!!!!!!!!!!");
  }

  if (*mode_flag != PRV_M) {
    // prepare mstatus
    mstatus_t *tmp_mstatus = (mstatus_t *)&csrCpt[0x300];
    tmp_mstatus->mpie = tmp_mstatus->mie;
    tmp_mstatus->mie = 0;
    tmp_mstatus->mpp = cpu.mode;
#ifdef CONFIG_RVH
    // checkpoint ub: mpp = 3, mpv = 1
    tmp_mstatus->mpv = cpu.v;
#endif

    // prepare mepc
    mepc_t *mepc_for_cpt = (mepc_t *)&csrCpt[0x341];
    mepc_for_cpt->val = cpu.pc;

  }

  Log("Writing CSR to checkpoint memory @host_paddr [0x%lx, 0x%lx) @mem_layout_addr [0x%lx, 0x%lx)", buffer_start,
      buffer_start + 4096 * 8, cpt_percpu_layout->csr_reg_cpt_addr, cpt_percpu_layout->csr_reg_cpt_addr + 4096 * 8);

  // time
  buffer_start = (uint64_t)serialize_base_addr + cpt_percpu_layout->mtime_cpt_addr;
  auto *mtime = (uint64_t *)(buffer_start);
  extern word_t paddr_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr);
  *mtime = ::paddr_read(CLINT_MMIO+0xBFF8, 8, MEM_TYPE_READ, MEM_TYPE_READ, MODE_M, CLINT_MMIO+0xBFF8);
  Log("Record time: 0x%lx to checkpoint memory @host_paddr [0x%lx, 0x%lx) @mem_layout_addr: [0x%lx, 0x%lx)", *mtime,
      buffer_start, buffer_start + 8, cpt_percpu_layout->mtime_cpt_addr, cpt_percpu_layout->mtime_cpt_addr + 8);

  buffer_start = (uint64_t)serialize_base_addr + cpt_percpu_layout->mtime_cmp_cpt_addr;
  auto *mtime_cmp = (uint64_t *)(buffer_start) + (0 * 8);
  *mtime_cmp = ::paddr_read(CLINT_MMIO+0x4000, 8, MEM_TYPE_READ, MEM_TYPE_READ, MODE_M, CLINT_MMIO+0x4000);
  Log("Record time_cmp flag: 0x%lx to checkpoint memory @host_paddr [0x%lx, 0x%lx) @mem_layout_addr: [0x%lx, 0x%lx)",
      *mtime_cmp, buffer_start, buffer_start + 8, cpt_percpu_layout->mtime_cmp_cpt_addr,
      cpt_percpu_layout->mtime_cmp_cpt_addr + 8);

  regDumped = true;
}
#else
void Serializer::serializeRegs() {}
#endif

void Serializer::serialize(uint64_t inst_count, bool write_to_flash) {
#ifdef CONFIG_MEM_COMPRESS
  checkpoint_header cpt_header = default_cpt_header;
  single_core_rvgc_rvv_rvh_memlayout cpt_percpu_layout = default_cpt_percpu_layout;
  uint64_t serialize_reg_base_addr;
  encode_cpt_header(&cpt_header, &cpt_percpu_layout);

  if (write_to_flash) {
    serialize_reg_base_addr = cpt_header.cpt_offset + (uint64_t)get_flash_base();
  } else {
    serialize_reg_base_addr = cpt_header.cpt_offset + (uint64_t)get_pmem();
  }

  serializeRegs(write_to_flash, (uint8_t *)serialize_reg_base_addr, &cpt_percpu_layout);
  serializePMem(inst_count, write_to_flash, get_pmem(), get_flash_base());
#else
  xpanic("You should enable CONFIG_MEM_COMPRESS in menuconfig");
#endif
}

void Serializer::init() {
  if  (checkpoint_state == SimpointCheckpointing) {
    assert(checkpoint_interval);
    intervalSize = checkpoint_interval;
    warmupIntervalSize = warmup_interval;
    Log("Taking simpoint checkpionts with profiling interval %lu", checkpoint_interval);

    auto simpoints_file = fstream(pathManager.getSimpointPath() + "simpoints0", std::ios::in);
    auto weights_file = fstream(pathManager.getSimpointPath() + "weights0", std::ios::in);
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

  } else if (checkpoint_state == UniformCheckpointing || checkpoint_state == ManualUniformCheckpointing) {
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
      } else {
        uint64_t next_point = (simpoint2Weights.begin()->first * intervalSize) <= warmupIntervalSize ? 0 : (simpoint2Weights.begin()->first * intervalSize) - warmupIntervalSize;
        if (num_insts >= next_point) {
          Log("Should take cpt now: %lu next point %lu", num_insts, next_point);
          return true;
        } else if (num_insts % intervalSize == 0) {
          Log("First cpt @ %lu, now: %lu", next_point, num_insts);
          break;
        } else {
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

  } else if (checkpoint_state == ManualUniformCheckpointing || checkpoint_state == UniformCheckpointing) {
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
  Log("set next index %ld", index);
  return index;
}

extern "C" {

void encode_cpt_header(checkpoint_header *cpt_header, single_core_rvgc_rvv_rvh_memlayout *cpt_percpu_layout){
  assert(cpt_header_encode(get_flash_base(), cpt_header, cpt_percpu_layout));
}

void init_serializer() {
  serializer.init();
}

bool try_take_cpt(uint64_t icount, bool using_gcpt_mmio) {
  if (serializer.instrsCouldTakeCpt(icount)) {
    serializer.serialize(icount, using_gcpt_mmio);
    serializer.notify_taken(icount);
    return true;
  }
  return false;
}

void serialize_reg_to_mem(bool using_gcpt_mmio) {
//  delete for now
//  serializer.serializeRegs(using_gcpt_mmio, serialize_base_addr, cpt_percpu_layout);
}

}
