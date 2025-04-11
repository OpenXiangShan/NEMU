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

#include <cassert>
#include <checkpoint/cpt_env.h>
#include <checkpoint/path_manager.h>
#include <checkpoint/serializer.h>
#include <cstdio>
#include <profiling/profiling_control.h>

#include "../isa/riscv64/local-include/csr.h"
#include <common.h>
#include <isa.h>

#include <cinttypes>
#include <iostream>
#include <limits>
#include <string>
#include <sys/cdefs.h>
#include <zlib.h>

#include <fcntl.h>
#include <fstream>
#include <gcpt_restore/src/restore_rom_addr.h>
#include <zstd.h>
#ifdef CONFIG_LIBCHECKPOINT_RESTORER
#include "pb.h"
#include "pb_encode.h"
#include <cpt_default_values.h>
#endif

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::numeric_limits;
using std::string;
using std::to_string;
#ifdef CONFIG_LIBCHECKPOINT_RESTORER
// protocol buffers are Google's language-neural,
// latform-neutral, extensible mechanism for serializing
// structured data. Excerpt from https://protobuf.dev/"
// We use protobuf to encode (NEMU) and decode (LibCheckpoint)
// our checkpoint layout.
CheckpointMetaData::CheckpointMetaData():
  default_header(multicore_default_header),
  default_single_core_memlayout(default_qemu_memlayout)
{
}

CheckpointMetaData checkpoint_meta_data;

checkpoint_header CheckpointMetaData::get_default_header(){
  return default_header;
}

single_core_rvgc_rvv_rvh_memlayout CheckpointMetaData::get_default_memlayout(){
  return default_single_core_memlayout;
}

bool CheckpointMetaData::header_encode(pb_ostream_t *output_stream, checkpoint_header *default_header){
  bool state = true;
  state = pb_encode_ex(output_stream, checkpoint_header_fields, default_header, PB_ENCODE_NULLTERMINATED);

  if (!state) {
    printf("Log: header encode error %s\n", output_stream->errmsg);
  }

  return state;
}

bool CheckpointMetaData::memlayout_encode(pb_ostream_t *output_stream, single_core_rvgc_rvv_rvh_memlayout * default_memlayout){
  bool state = true;
  state = pb_encode_ex(output_stream, single_core_rvgc_rvv_rvh_memlayout_fields, default_memlayout, PB_ENCODE_NULLTERMINATED);

  if (!state) {
    printf("Log: memlayout encode error %s\n", output_stream->errmsg);
  }

  return state;
}

bool CheckpointMetaData::encode(uint8_t *mem_buffer, uint64_t buffer_size) {

  bool state = true;
  pb_ostream_t output_stream = pb_ostream_from_buffer(mem_buffer, buffer_size);
  state = header_encode(&output_stream, &default_header);
  assert(state);
  memlayout_encode(&output_stream, &default_single_core_memlayout);
  assert(state);

  return state;
}

uint8_t* CheckpointMetaData::get_checkpoint_data_address(uint64_t memory_start_address){
  return (uint8_t*)(default_header.cpt_offset + memory_start_address);
}


#endif

Serializer::Serializer() :
    int_reg_cpt_addr(INT_REG_CPT_ADDR-BOOT_CODE),
    int_reg_done(INT_REG_DONE-BOOT_CODE),
    float_reg_cpt_addr(FLOAT_REG_CPT_ADDR-BOOT_CODE),
    float_reg_done(FLOAT_REG_DONE-BOOT_CODE),
    csr_reg_cpt_addr(CSR_REG_CPT_ADDR-BOOT_CODE),
    csr_reg_done(CSR_REG_DONE-BOOT_CODE),
    vector_reg_cpt_addr(VECTOR_REG_CPT_ADDR-BOOT_CODE),
    vector_reg_done(VECTOR_REG_DONE-BOOT_CODE),
    magic_number_cpt_addr(BOOT_FLAG_ADDR-BOOT_CODE),
    pc_cpt_addr(PC_CPT_ADDR-BOOT_CODE),
    mode_cpt_addr(MODE_CPT_ADDR-BOOT_CODE),
    mtime_cpt_addr(MTIME_CPT_ADDR-BOOT_CODE),
    mtime_cmp_cpt_addr(MTIME_CMP_CPT_ADDR-BOOT_CODE),
    misc_done_cpt_addr(MISC_DONE_CPT_ADDR-BOOT_CODE)
{
}

extern "C" {
uint8_t *get_pmem();
word_t paddr_read(paddr_t addr, int len, int type, int trap_type, int mode, vaddr_t vaddr);
uint8_t *guest_to_host(paddr_t paddr);
#include <debug.h>
#include <device/flash.h>
#include <checkpoint/semantic_point.h>
extern void log_buffer_flush();
extern void log_file_flush();
extern unsigned long MEMORY_SIZE;
}

#ifdef CONFIG_MEM_COMPRESS
void Serializer::serializePMem(uint64_t inst_count, uint8_t *pmem_addr, uint8_t *flash_addr) {
  // We must dump registers before memory to store them in the Generic Arch CPT
  assert(regDumped);
  const size_t PMEM_SIZE = MEMORY_SIZE;
  assert(pmem_addr);
  Log("Host physical address: %p size: %lx", pmem_addr, PMEM_SIZE);

#ifdef CONFIG_HAS_FLASH
  const size_t FLASH_SIZE = get_flash_size();
  assert(flash_addr);
  Log("Host flash address: %p size: %lx", flash_addr, FLASH_SIZE);
  string flash_file_path;
#endif

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
#ifdef CONFIG_HAS_FLASH
    flash_file_path = base_file_path + "_flash_.gz";
    gzFile flash_compressed_mem;
#endif
    memory_file_path = base_file_path + "_memory_.gz";
    gzFile memory_compressed_file = gzopen(memory_file_path.c_str(), "wb");
    if (memory_compressed_file == nullptr) {
      cerr << "Failed to open " << memory_file_path << endl;
      xpanic("Can't open file %s!\n", memory_file_path.c_str());
    } else {
      cout << "Opening " << memory_file_path << " as memory checkpoint output file" << endl;
    }

    uint64_t pass_size = 0;
    if (store_cpt_in_flash) {
#ifdef CONFIG_HAS_FLASH
      flash_compressed_mem = gzopen(flash_file_path.c_str(), "wb");
      if (flash_compressed_mem == nullptr) {
        cerr << "Failed to open " << flash_file_path << endl;
        xpanic("Can't open file %s!\n", flash_file_path.c_str());
      } else {
        cout << "Opening " << flash_file_path << " as flash checkpoint output file" << endl;
      }

      for (uint64_t flash_written = 0; flash_written < FLASH_SIZE; flash_written += pass_size) {
        pass_size = numeric_limits<int>::max() < ((int64_t)FLASH_SIZE - (int64_t)flash_written)
                      ? numeric_limits<int>::max()
                      : ((int64_t)FLASH_SIZE - (int64_t)flash_written);
        if (gzwrite(flash_compressed_mem, flash_addr + flash_written, (uint32_t)pass_size) != (int)pass_size) {
          xpanic("Write failed on file: %s\n", flash_file_path.c_str());
        }
        Log("Written 0x%lx bytes\n", pass_size);
      }

      pass_size = 0;
#endif
    }

    for (uint64_t written = 0; written < PMEM_SIZE; written += pass_size) {
      pass_size = numeric_limits<int>::max() < ((int64_t)PMEM_SIZE - (int64_t)written)
                    ? numeric_limits<int>::max()
                    : ((int64_t)PMEM_SIZE - (int64_t)written);

      if (gzwrite(memory_compressed_file, pmem_addr + written, (uint32_t)pass_size) != (int)pass_size) {
        xpanic("Write failed on file %s\n", memory_file_path.c_str());
      }
      Log("Written 0x%lx bytes\n", pass_size);
    }

    if(store_cpt_in_flash){
#ifdef CONFIG_HAS_FLASH
      if (gzclose(flash_compressed_mem)) {
        xpanic("Close failed on file %s\n", flash_file_path.c_str());
      }
#endif
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

    __attribute__((unused))
    size_t flash_compress_size = 0;
    __attribute__((unused))
    FILE *flash_compress_file = NULL;
    __attribute__((unused))
    size_t flash_fw_size = 0;

#ifdef CONFIG_HAS_FLASH
    flash_file_path += base_file_path + "_flash_.zstd";
    Log("Opening %s as checkpoint output file", flash_file_path.c_str());

    size_t const flash_compress_buffer_size = ZSTD_compressBound(FLASH_SIZE);
    uint8_t *const flash_compress_buffer = (uint8_t*)malloc(flash_compress_buffer_size);
    assert(flash_compress_buffer);

    // compress flash device memory
    if (store_cpt_in_flash) {
      flash_compress_size = ZSTD_compress(flash_compress_buffer, flash_compress_buffer_size, flash_addr, FLASH_SIZE, 1);
      assert(flash_compress_size <= flash_compress_buffer_size && flash_compress_size != 0);
      Log("compress flash success, compress size %ld", flash_compress_size);

      flash_compress_file = fopen(flash_file_path.c_str(), "wb");
      flash_fw_size = fwrite(flash_compress_buffer, 1, flash_compress_size, flash_compress_file);
    }
#endif

    size_t memory_compress_size = ZSTD_compress(memory_compress_buffer, memory_compress_buffer_size, pmem_addr, PMEM_SIZE, 1);
    assert(memory_compress_size <= memory_compress_buffer_size && memory_compress_size != 0);
    Log("pmem compress success, compress size %ld", memory_compress_size);

    FILE *memory_compress_file = fopen(memory_file_path.c_str(), "wb");
    size_t memory_fw_size = fwrite(memory_compress_buffer, 1, memory_compress_size, memory_compress_file);

    if (flash_fw_size != flash_compress_size || memory_fw_size != memory_compress_size) {
      if(store_cpt_in_flash){
#ifdef CONFIG_HAS_FLASH
        fclose(flash_compress_file);
#endif
      }
      fclose(memory_compress_file);
#ifdef CONFIG_HAS_FLASH
      free(flash_compress_buffer);
#endif
      free(memory_compress_buffer);
#ifdef CONFIG_HAS_FLASH
      printf("file write error: %s : %s\n", flash_file_path.c_str(), strerror(errno));
#endif
      xpanic("file write error: %s : %s\n", memory_file_path.c_str(), strerror(errno));
    }

    if(store_cpt_in_flash){
#ifdef CONFIG_HAS_FLASH
      if (fclose(flash_compress_file)) {
        free(flash_compress_buffer);
        xpanic("file close error: %s : %s \n", flash_file_path.c_str(), strerror(errno));
      }
#endif
    }

    if (fclose(memory_compress_file)) {
      free(memory_compress_buffer);
      xpanic("file close error: %s : %s \n", memory_file_path.c_str(), strerror(errno));
    }

#ifdef CONFIG_HAS_FLASH
    free(flash_compress_buffer);
#endif
    free(memory_compress_buffer);

  } else {
    xpanic("You need to specify the compress file format using: --checkpoint-format\n");
  }

  Log("Checkpoint done!\n");
  regDumped = false;
}
#else
void Serializer::serializePMem(uint64_t inst_count, uint8_t *pmem_addr, uint8_t *flash_addr) {}
#endif

#ifdef CONFIG_MEM_COMPRESS
extern void csr_writeback();

void Serializer::serializeRegs(uint8_t* serialize_base_addr) {
  auto *intRegCpt = (uint64_t *) (serialize_base_addr + int_reg_cpt_addr);
  for (unsigned i = 0; i < 32; i++) {
    *(intRegCpt + i) = cpu.gpr[i]._64;
  }
  Log("Writing int registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)", INT_REG_CPT_ADDR,
      INT_REG_CPT_ADDR + 32 * 8, int_reg_cpt_addr, int_reg_cpt_addr + 32 * 8);

#ifndef CONFIG_FPU_NONE
  auto *floatRegCpt = (uint64_t *)(serialize_base_addr + float_reg_cpt_addr);
  for (unsigned i = 0; i < 32; i++) {
    *(floatRegCpt + i) = cpu.fpr[i]._64;
  }
  Log("Writing float registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)", FLOAT_REG_CPT_ADDR,
      FLOAT_REG_CPT_ADDR + 32 * 8, float_reg_cpt_addr, float_reg_cpt_addr + 32 * 8);
#endif  // CONFIG_FPU_NONE

#ifdef CONFIG_RVV
  auto *vectorRegCpt = (uint64_t *) (serialize_base_addr + vector_reg_cpt_addr);
  for (unsigned i = 0; i < 32; i++) {
    for (unsigned j = 0; j < VENUM64; j++) {
      *(vectorRegCpt + (i * VENUM64) + j)=cpu.vr[i]._64[j];
    }
  }
  Log("Writing Vector registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)",
      FLOAT_REG_CPT_ADDR, FLOAT_REG_CPT_ADDR + 32 * 8,
      vector_reg_cpt_addr, vector_reg_cpt_addr + 32 * 8 * VENUM64
      );
#endif // CONFIG_RVV


  auto *pc = (uint64_t *) (serialize_base_addr + pc_cpt_addr);
  *pc = cpu.pc;
  Log("Writing PC: 0x%lx at addr 0x%x", cpu.pc, PC_CPT_ADDR);


  //  csr_writeback();
  auto *csrCpt = (uint64_t *)(serialize_base_addr + csr_reg_cpt_addr);
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
  mstatus_prepare->spie=mstatus_prepare->sie;
  mstatus_prepare->sie=0;
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
      csr_reg_cpt_addr, csr_reg_cpt_addr + 4096 * 8
      );


  auto *flag = (uint64_t *)(serialize_base_addr + magic_number_cpt_addr);
  *flag = CPT_MAGIC_BUMBER;
  Log("Touching Flag: 0x%x at addr 0x%x", CPT_MAGIC_BUMBER, BOOT_FLAG_ADDR);

  auto *mode_flag = (uint64_t *) (serialize_base_addr + mode_cpt_addr);
  *mode_flag = cpu.mode;
  Log("Record mode flag: 0x%lx at addr 0x%x", cpu.mode, MODE_CPT_ADDR);

  auto *mtime = (uint64_t *) (serialize_base_addr + mtime_cpt_addr);
  extern word_t paddr_read(paddr_t addr, int len, int type, int mode, vaddr_t vaddr);
  *mtime = ::paddr_read(CLINT_MMIO+0xBFF8, 8, MEM_TYPE_READ, MEM_TYPE_READ, MODE_M, CLINT_MMIO+0xBFF8);
  Log("Record time: 0x%lx at addr 0x%x", cpu.mode, MTIME_CPT_ADDR);

  auto *mtime_cmp = (uint64_t *) (serialize_base_addr + mtime_cmp_cpt_addr);
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

  __attribute__((unused))
  uint64_t serialize_buffer_size;

  if (store_cpt_in_flash) {
    IFDEF(CONFIG_HAS_FLASH, serialize_reg_base_addr = get_flash_base(); assert(get_flash_size() >= 1000000U); serialize_buffer_size = get_flash_size(););
    IFNDEF(CONFIG_HAS_FLASH, Log("Please enable the flash device to activate the functionality of saving checkpoints to flash."); assert(0));
  } else {
    serialize_reg_base_addr = get_pmem();
    assert(MEMORY_SIZE >= 1000000U);
    serialize_buffer_size = MEMORY_SIZE;
  }
  assert(serialize_reg_base_addr);
#ifdef CONFIG_LIBCHECKPOINT_RESTORER
  assert(checkpoint_meta_data.encode(serialize_reg_base_addr, serialize_buffer_size));

  serialize_reg_base_addr = checkpoint_meta_data.get_checkpoint_data_address((uint64_t)serialize_reg_base_addr);

#endif
  serializeRegs(serialize_reg_base_addr);
#ifdef CONFIG_HAS_FLASH
  serializePMem(inst_count, get_pmem(), get_flash_base());
#else
  serializePMem(inst_count, get_pmem(), NULL);
#endif

#else
  xpanic("You should enable CONFIG_MEM_COMPRESS in menuconfig");
#endif
}

#ifdef CONFIG_LIBCHECKPOINT_RESTORER

#define USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(variable_name) \
  this->variable_name = checkpoint_meta_data.get_default_memlayout().variable_name;

void Serializer::init(bool store_cpt_in_flash, bool enable_libcheckpoint) {
#else
void Serializer::init(bool store_cpt_in_flash) {
#endif
  this->store_cpt_in_flash = store_cpt_in_flash;

#ifdef CONFIG_LIBCHECKPOINT_RESTORER
  this->enable_libcheckpoint = enable_libcheckpoint;

  if (this->enable_libcheckpoint) {
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(int_reg_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(int_reg_done);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(float_reg_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(float_reg_done);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(csr_reg_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(csr_reg_done);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(vector_reg_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(vector_reg_done);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(magic_number_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(pc_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(mode_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(mtime_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(mtime_cmp_cpt_addr);
    USING_METADATA_OVERRIDE_DEFAULT_MEMLAYOUT(misc_done_cpt_addr);
  }

#endif
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
    case SemanticCheckpointing:
      return check_semantic_point();
    case CheckpointOnNEMUTrap:
      return true;
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
#ifdef CONFIG_LIBCHECKPOINT_RESTORER
void init_serializer(bool store_cpt_in_flash, bool enable_libcheckpoint) {
  serializer.init(store_cpt_in_flash, enable_libcheckpoint);
}
#else
void init_serializer(bool store_cpt_in_flash) {
  serializer.init(store_cpt_in_flash);
}
#endif

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
