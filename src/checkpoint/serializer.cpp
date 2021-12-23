//
// Created by zyy on 2020/11/16.
//

#include "checkpoint/path_manager.h"
#include "checkpoint/serializer.h"
#include "../isa/riscv64/local-include/csr.h"
#include <device/map.h>

#include <cinttypes>
#include <iostream>
#include <zlib.h>
#include <limits>
#include <boost/algorithm/string/predicate.hpp>

#include <isa.h>
#include <fstream>
#include <memory/paddr.h>
#include <monitor/monitor.h>
#include <gcpt_restore/src/restore_rom_addr.h>

using namespace std;

Serializer::Serializer() :
    IntRegStartAddr(INT_REG_CPT_ADDR - BOOT_CODE),
    FloatRegStartAddr(FLOAT_REG_CPT_ADDR - BOOT_CODE),
    CSRStartAddr(CSR_CPT_ADDR - BOOT_CODE),
    PCAddr(PC_CPT_ADDR - BOOT_CODE),
    CptFlagAddr(BOOT_FLAGS - BOOT_CODE)
{

}

void Serializer::serializePMem(uint64_t inst_count) {
  // We must dump registers before memory to store them in the Generic Arch CPT
  assert(regDumped);

  if (simpoint2Weights.empty()) {
    Log("Error: serializePMem but simpoint2Weights is empty, exit");
    exit(0);
  }

  uint8_t *pmem = getPmem();
  string filepath;
  if (profiling_state == SimpointCheckpointing) {
#ifdef FLAT_CPTPATH
  std::string path_dir = pathManager.getOutputPath() + pathManager.getWorkloadName() + "_" + \
                        to_string(simpoint2Weights.begin()->first * intervalSize) + "_" + \
                        to_string(simpoint2Weights.begin()->second) + "/0/";
  if (!fs::exists(fs::path(path_dir))) {
    fs::create_directories(fs::path(path_dir));
    Log("Created %s\n", path_dir.c_str());
  }
  filepath = path_dir + "_" + \
                        to_string(simpoint2Weights.begin()->first * intervalSize) + "_.gz";
#else
  filepath = pathManager.getOutputPath() + "_" + \
                        to_string(simpoint2Weights.begin()->first * intervalSize) + "_" + \
                        to_string(simpoint2Weights.begin()->second) + "_.gz";
#endif
  } else {
      filepath = pathManager.getOutputPath() + "_" + \
                        to_string(inst_count) + "_.gz";
  }

  gzFile compressed_mem = gzopen(filepath.c_str(), "wb");
  if (compressed_mem == nullptr) {
    cerr << "Failed to open " << filepath << endl;
    panic("Can't open physical memory checkpoint file!\n");
  } else {
    cout << "Opening " << filepath << " as checkpoint output file" << endl;
  }

  uint64_t pass_size = 0;

  for (uint64_t written = 0; written < PMEM_SIZE; written += pass_size) {
    pass_size = numeric_limits<int>::max() < ((int64_t) PMEM_SIZE - (int64_t) written) ?
                numeric_limits<int>::max() : ((int64_t) PMEM_SIZE - (int64_t) written);

    if (gzwrite(compressed_mem, pmem + written, (uint32_t) pass_size) != (int) pass_size) {
      panic("Write failed on physical memory checkpoint file\n");
    }
    Log("Written 0x%x bytes\n", pass_size);
  }

  if (gzclose(compressed_mem)){
    panic("Close failed on physical memory checkpoint file\n");
  }
  Log("Checkpoint done!\n");
  regDumped = false;
}

extern void csr_writeback();

void Serializer::serializeRegs() {
  auto *intRegCpt = (uint64_t *) (getPmem() + IntRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(intRegCpt + i) = cpu.gpr[i]._64;
  }
  Log("Writing int registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)",
      INT_REG_CPT_ADDR, INT_REG_CPT_ADDR + 32 * 8,
      IntRegStartAddr, IntRegStartAddr + 32 * 8
      );


  auto *floatRegCpt = (uint64_t *) (getPmem() + FloatRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(floatRegCpt + i) = cpu.fpr[i]._64;
  }
  Log("Writing float registers to checkpoint memory @[0x%x, 0x%x) [0x%x, 0x%x)",
      FLOAT_REG_CPT_ADDR, FLOAT_REG_CPT_ADDR + 32 * 8,
      FloatRegStartAddr, FloatRegStartAddr + 32 * 8
      );


  auto *pc = (uint64_t *) (getPmem() + PCAddr);
  *pc = cpu.pc;
  Log("Writing PC: 0x%lx at addr 0x%x", cpu.pc, PC_CPT_ADDR);


//  csr_writeback();
  auto *csrCpt = (uint64_t *) (getPmem() + CSRStartAddr);
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


  auto *flag = (uint64_t *) (getPmem() + CptFlagAddr);
  *flag = CPT_MAGIC_BUMBER;
  Log("Touching Flag: 0x%x at addr 0x%x", CPT_MAGIC_BUMBER, BOOT_FLAGS);

  auto *mode_flag = (uint64_t *) (getPmem() + CptFlagAddr + 8);
  *mode_flag = cpu.mode;
  Log("Record mode flag: 0x%x at addr 0x%x", cpu.mode, BOOT_FLAGS+8);

  auto *mtime = (uint64_t *) (getPmem() + CptFlagAddr + 16);
  *mtime = paddr_read(CLINT_MMIO+0xBFF8,8);
  Log("Record time: 0x%x at addr 0x%x", cpu.mode, BOOT_FLAGS+16);

  auto *mtime_cmp = (uint64_t *) (getPmem() + CptFlagAddr + 24);
  *mtime_cmp = paddr_read(CLINT_MMIO+0x4000,8);
  Log("Record time: 0x%x at addr 0x%x", cpu.mode, BOOT_FLAGS+24);

  regDumped = true;
}

void Serializer::serialize(uint64_t inst_count) {
  pathManager.setOutputDir();
//  isa_reg_display();
  serializeRegs();
  serializePMem(inst_count);

//  isa_reg_display();
}

void Serializer::unserialize(const char *file) {

  if (!boost::algorithm::ends_with(file, ".gz")) {
    // process raw binary
    FILE *fp = fopen(file, "rb");
    if (fp == NULL) Log("Checkpoint not found\n");
    Assert(fp, "Can not open '%s'", file);

    Log("Opening restorer file: %s", file);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size < PMEM_SIZE) {
      Log("Cpt size = %ld is too large, only load 0xlu", size, PMEM_SIZE);
      size = PMEM_SIZE;
    }

    fseek(fp, 0, SEEK_SET);
    int ret = fread(guest_to_host(RESTORER_START), size, 1, fp);
    assert(ret == 1);

  } else {

    gzFile compressed_mem = gzopen(file, "rb");
    if (compressed_mem == nullptr) {
      panic("Can't open physical memory checkpoint file '%s'", file);
    }

    uint64_t curr_size = 0;
    const uint32_t chunk_size = 16384;
    long *temp_page = new long[chunk_size];
    long *pmem_current;

    while (curr_size < PMEM_SIZE) {
      uint32_t bytes_read = gzread(compressed_mem, temp_page, chunk_size);
      if (bytes_read == 0) {
        break;
      }
      assert(bytes_read % sizeof(long) == 0);
      for (uint32_t x = 0; x < bytes_read / sizeof(long); x++) {
        if (*(temp_page + x) != 0) {
          pmem_current = (long*)(getPmem() + curr_size + x * sizeof(long));
          *pmem_current = *(temp_page + x);
        }
      }
      curr_size += bytes_read;
    }
    Log("Read %lu bytes from gz stream in total", curr_size);

    extern bool xpoint_profiling_started;
    xpoint_profiling_started = true;

    delete [] temp_page;

    if (gzclose(compressed_mem)) {
      panic("Error closing '%s'\n", file);
    }
  }
}

void Serializer::init() {
  if  (profiling_state == SimpointCheckpointing) {
    assert(checkpoint_interval);
    intervalSize = checkpoint_interval;
    Log("Taking checkpionts with interval %lu", checkpoint_interval);

    auto simpoints_file = fstream(pathManager.getSimpointPath() + "simpoints0");
    auto weights_file = fstream(pathManager.getSimpointPath() + "weights0");
    Log("simpoint file: %s", pathManager.getSimpointPath() + "simpoints0");
    Log("weight file: %s", pathManager.getSimpointPath() + "weights0");
    assert(!simpoints_file.bad());
    assert(!weights_file.bad());

    uint64_t simpoint_location, simpoint_id, weight_id;
    double weight;

    while (simpoints_file >> simpoint_location >> simpoint_id) {
      assert(weights_file >> weight >> weight_id);
      assert(weight_id == simpoint_id);
      simpoint2Weights[simpoint_location] = weight;

      Log("Simpoint %i: @ %lu, weight: %f", simpoint_id, simpoint_location, weight);
    }
    assert(!simpoint2Weights.empty());
  } else if (checkpointTaking) {
    assert(checkpoint_interval);
    intervalSize = checkpoint_interval;
    Log("Taking normal checkpionts with interval %lu", checkpoint_interval);
    nextNormalPoint = intervalSize;
  }
}

bool Serializer::shouldTakeCpt(uint64_t num_insts) {
  if ((profiling_state != SimpointCheckpointing || simpoint2Weights.empty()) && !checkpointTaking) {
    return false;
  }
  extern bool xpoint_profiling_started;

  if (profiling_state == SimpointCheckpointing) {
      uint64_t next_point = ((simpoint2Weights.begin()->first >= 1) ? (simpoint2Weights.begin()->first - 1) : 0) * intervalSize + 100000;
      if (num_insts >= next_point) {
          //Log("Should take cpt now: %lu", num_insts);
          return true;
      } else if (num_insts % intervalSize == 0) {
          Log("First cpt @ %lu, now: %lu",
                  next_point, num_insts);
      }
  } else if (checkpointTaking && xpoint_profiling_started){
      if (num_insts >= nextNormalPoint) {
          //Log("Should take cpt now: %lu", num_insts);
          return true;
      }
  }
  return false;
}

void Serializer::notify_taken(uint64_t i) {
  Log("Taking checkpoint @ instruction count %lu", i);
  if (profiling_state == SimpointCheckpointing) {
    simpoint2Weights.erase(simpoint2Weights.begin());
    if (!simpoint2Weights.empty()) {
        pathManager.incCptID();
    } else {
      Log("All the checkpoints have been taken, exit...");
      exit(0);
    }

  } else if (checkpointTaking) {
    nextNormalPoint += intervalSize;
    pathManager.incCptID();
  }
}

Serializer serializer;

void init_serializer() {
  serializer.init();
}
