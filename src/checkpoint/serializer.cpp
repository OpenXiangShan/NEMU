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

#include <cinttypes>
#include <iostream>
#include <limits>
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

Serializer::Serializer() :
    IntRegStartAddr(INT_REG_CPT_ADDR-BOOT_CODE),
    IntRegDoneFlag(INT_REG_DONE-BOOT_CODE),
    FloatRegStartAddr(FLOAT_REG_CPT_ADDR-BOOT_CODE),
    FloatRegDoneFlag(FLOAT_REG_DONE-BOOT_CODE),
    CSRStartAddr(CSR_REG_CPT_ADDR-BOOT_CODE),
    CSRSDoneFlag(CSR_REG_DONE-BOOT_CODE),
    VecRegStartAddr(VECTOR_REG_CPT_ADDR-BOOT_CODE),
    VecRegDoneFlag(VECTOR_REG_DONE-BOOT_CODE),
    CptFlagAddr(BOOT_FLAG_ADDR-BOOT_CODE),
    PCAddr(PC_CPT_ADDR-BOOT_CODE),
    MODEAddr(MODE_CPT_ADDR-BOOT_CODE),
    MTIMEAddr(MTIME_CPT_ADDR-BOOT_CODE),
    MTIMECMPAddr(MTIME_CMP_CPT_ADDR-BOOT_CODE),
    MISCDoneFlag(MISC_DONE_CPT_ADDR-BOOT_CODE)
{
}

extern "C" {
uint8_t *get_pmem();
word_t paddr_read(paddr_t addr, int len, int type, int trap_type, int mode, vaddr_t vaddr);
uint8_t *guest_to_host(paddr_t paddr);
#include <debug.h>
#include <device/flash.h>
extern void log_buffer_flush();
extern void log_file_flush();
extern unsigned long MEMORY_SIZE;
}

#ifdef CONFIG_MEM_COMPRESS
void Serializer::serializePMem(uint64_t inst_count) {
  // We must dump registers before memory to store them in the Generic Arch CPT
  assert(regDumped);
  const size_t PMEM_SIZE = MEMORY_SIZE;
  uint8_t *pmem = get_pmem();

  if (restorer) {
  FILE *restore_fp = fopen(restorer, "rb");
  if (!restore_fp) {
    xpanic("Cannot open restorer %s\n", restorer);
  }

  uint32_t restorer_size = 0xf00;
  fseek(restore_fp, 0, SEEK_SET);
  assert(restorer_size == fread(pmem, 1, restorer_size, restore_fp));
  fclose(restore_fp);

  Log("Put gcpt restorer %s to start of pmem", restorer);
  }

  string base_file_path;
  string memory_file_path;

  if (checkpoint_state == SimpointCheckpointing) {
    base_file_path = pathManager.getOutputPath() + "_" + to_string(simpoint2Weights.begin()->first) + "_" +
               to_string(simpoint2Weights.begin()->second);
  } else {
    base_file_path = pathManager.getOutputPath() + "_" + to_string(inst_count);
  }

  if (compress_file_format == GZ_FORMAT) {
    Log("Using GZ format generate checkpoint");
    memory_file_path = base_file_path + "_memory_.gz";
    gzFile memory_compressed_file = gzopen(memory_file_path.c_str(), "wb");
    if (memory_compressed_file == nullptr) {
      cerr << "Failed to open " << memory_file_path << endl;
      xpanic("Can't open physical memory checkpoint file!\n");
    } else {
      cout << "Opening " << memory_file_path << " as checkpoint output file" << endl;
    }

    uint64_t pass_size = 0;

    for (uint64_t written = 0; written < PMEM_SIZE; written += pass_size) {
      pass_size = numeric_limits<int>::max() < ((int64_t)PMEM_SIZE - (int64_t)written)
                    ? numeric_limits<int>::max()
                    : ((int64_t)PMEM_SIZE - (int64_t)written);

      if (gzwrite(memory_compressed_file, pmem + written, (uint32_t)pass_size) != (int)pass_size) {
        xpanic("Write failed on physical memory checkpoint file\n");
      }
      Log("Written 0x%lx bytes\n", pass_size);
    }

    if (gzclose(memory_compressed_file)) {
      xpanic("Close failed on physical checkpoint file\n");
    }

  } else if (compress_file_format == ZSTD_FORMAT) {
    Log("Using ZSTD format generate checkpoint");

    memory_file_path += base_file_path + "_memory_.zstd";
    Log("Opening %s as memory output file", memory_file_path.c_str());

    // zstd compress
    size_t memory_size = PMEM_SIZE;
    size_t const memory_compress_buffer_size = ZSTD_compressBound(memory_size);
    uint8_t *const memory_compress_buffer = (uint8_t*)malloc(memory_compress_buffer_size);
    assert(memory_compress_buffer);




    // compress flash device memory
    size_t memory_compress_size = ZSTD_compress(memory_compress_buffer, memory_compress_buffer_size, pmem, PMEM_SIZE, 1);
    assert(memory_compress_size <= memory_compress_buffer_size && memory_compress_size != 0);
    Log("pmem compress success, compress size %ld", memory_compress_size);

    FILE *memory_compress_file = fopen(memory_file_path.c_str(), "wb");
    size_t memory_fw_size = fwrite(memory_compress_buffer, 1, memory_compress_size, memory_compress_file);

    if (memory_fw_size != memory_compress_size) {
      fclose(memory_compress_file);
      free(memory_compress_buffer);
      xpanic("file write error: %s : %s\n", memory_file_path.c_str(), strerror(errno));
    }

    if (fclose(memory_compress_file)) {
      free(memory_compress_buffer);
      xpanic("file close error: %s : %s \n", base_file_path.c_str(), strerror(errno));
    }

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

void Serializer::serializeRegs(uint8_t* serialize_base_addr) {
  auto *intRegCpt = (uint64_t *) (serialize_base_addr + IntRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(intRegCpt + i) = cpu.gpr[i]._64;
  }
  Log("Writing int registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)", INT_REG_CPT_ADDR,
      INT_REG_CPT_ADDR + 32 * 8, IntRegStartAddr, IntRegStartAddr + 32 * 8);

#ifndef CONFIG_FPU_NONE
  auto *floatRegCpt = (uint64_t *)(serialize_base_addr + FloatRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(floatRegCpt + i) = cpu.fpr[i]._64;
  }
  Log("Writing float registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)", FLOAT_REG_CPT_ADDR,
      FLOAT_REG_CPT_ADDR + 32 * 8, FloatRegStartAddr, FloatRegStartAddr + 32 * 8);
#endif  // CONFIG_FPU_NONE

#ifdef CONFIG_RVV
  auto *vectorRegCpt = (uint64_t *) (serialize_base_addr + VecRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    for (unsigned j = 0; j < VENUM64; j++) {
      *(vectorRegCpt + (i * VENUM64) + j)=cpu.vr[i]._64[j];
    }
  }
  Log("Writing Vector registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)",
      FLOAT_REG_CPT_ADDR, FLOAT_REG_CPT_ADDR + 32 * 8,
      VecRegStartAddr, VecRegStartAddr + 32 * 8 * VENUM64
      );
#endif // CONFIG_RVV


  auto *pc = (uint64_t *) (serialize_base_addr + PCAddr);
  *pc = cpu.pc;
  Log("Writing PC: 0x%lx at addr 0x%x", cpu.pc, PC_CPT_ADDR);


  //  csr_writeback();
  auto *csrCpt = (uint64_t *)(serialize_base_addr + CSRStartAddr);
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

  //prepare mstatus
  mstatus_t *mstatus_prepare=(mstatus_t *)&csrCpt[0x300];
  mstatus_prepare->mpie=mstatus_prepare->mie;
  mstatus_prepare->mie=0;
  mstatus_prepare->mpp=cpu.mode;

#ifdef CONFIG_RVH
  // checkpoint ub: mpp = 3, mpv = 1
  mstatus_prepare->mpv=cpu.v;
#endif

  //prepare mepc
  mepc_t *mepc_prepare=(mepc_t*)&csrCpt[0x341];
  mepc_prepare->val=cpu.pc;

  Log("Writing CSR to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)",
      CSR_REG_CPT_ADDR, CSR_REG_CPT_ADDR + 4096 * 8,
      CSRStartAddr, CSRStartAddr + 4096 * 8
      );


  auto *flag = (uint64_t *)(serialize_base_addr + CptFlagAddr);
  *flag = CPT_MAGIC_BUMBER;
  Log("Touching Flag: 0x%x at addr 0x%x", CPT_MAGIC_BUMBER, BOOT_FLAG_ADDR);

  auto *mode_flag = (uint64_t *) (serialize_base_addr + MODEAddr);
  *mode_flag = cpu.mode;
  Log("Record mode flag: 0x%lx at addr 0x%x", cpu.mode, MODE_CPT_ADDR);

  auto *mtime = (uint64_t *) (serialize_base_addr + MTIMEAddr);
  extern word_t paddr_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr);
  *mtime = ::paddr_read(CLINT_MMIO+0xBFF8, 8, MEM_TYPE_READ, MEM_TYPE_READ, MODE_M, CLINT_MMIO+0xBFF8);
  Log("Record time: 0x%lx at addr 0x%x", cpu.mode, MTIME_CPT_ADDR);

  auto *mtime_cmp = (uint64_t *) (serialize_base_addr + MTIMECMPAddr);
  *mtime_cmp = ::paddr_read(CLINT_MMIO+0x4000, 8, MEM_TYPE_READ, MEM_TYPE_READ, MODE_M, CLINT_MMIO+0x4000);
  Log("Record time: 0x%lx at addr 0x%x", cpu.mode, MTIME_CMP_CPT_ADDR);

  regDumped = true;
}
#else
void Serializer::serializeRegs(uint8_t* serialize_base_addr) { }
#endif

void Serializer::serialize(uint64_t inst_count) {

#ifdef CONFIG_MEM_COMPRESS
  uint8_t* serialize_reg_base_addr = NULL;

  if (store_cpt_in_flash) {
    IFDEF(CONFIG_HAS_FLASH, serialize_reg_base_addr = get_flash_base());
    IFNDEF(CONFIG_HAS_FLASH, Log("Please enable the flash device to activate the functionality of saving checkpoints to flash."); assert(0));
  } else {
    serialize_reg_base_addr = get_pmem();
  }
  assert(serialize_reg_base_addr);

  serializeRegs(serialize_reg_base_addr);
  serializePMem(inst_count);

#else
  xpanic("You should enable CONFIG_MEM_COMPRESS in menuconfig");
#endif
}

void Serializer::init(bool store_cpt_in_flash) {
  this->store_cpt_in_flash = store_cpt_in_flash;

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

void init_serializer(bool store_cpt_in_flash) {
  serializer.init(store_cpt_in_flash);
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
  serializer.serializeRegs(get_pmem());
}

}
