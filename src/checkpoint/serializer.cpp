//
// Created by zyy on 2020/11/16.
//

#include "checkpoint/serializer.h"

#include <cinttypes>
#include <iostream>
#include <zlib.h>
#include <limits>

#include <isa.h>

#include <memory/paddr.h>
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


void Serializer::serializePMem() {
  // We must dump registers before memory to store them in the Generic Arch CPT
  assert(regDumped);

  uint8_t *pmem = getPmem();
  string filename = taskName + "." + phaseName + "_" + weightIndicator + "_.gz";
  string filepath = outputPath + "/" +filename;

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
  }

  if (gzclose(compressed_mem)){
    panic("Close failed on physical memory checkpoint file\n");
  }
  Log("Checkpoint done!\n");
}

extern void csr_writeback();

void Serializer::serializeRegs() {
  Log("Writing integer registers to checkpoint memory\n");
  auto *intRegCpt = (uint64_t *) (getPmem() + IntRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(intRegCpt + i) = cpu.gpr[i]._64;
  }

  Log("Writing float registers to checkpoint memory\n");
  auto *floatRegCpt = (uint64_t *) (getPmem() + FloatRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(floatRegCpt + i) = cpu.fpr[i]._64;
  }

  Log("Writing CSRs to checkpoint memory\n");
  csr_writeback();
  auto *csrCpt = (uint64_t *) (getPmem() + CSRStartAddr);
  for (unsigned i = 0; i < 4096; i++) {
    *(csrCpt + i) = csr_array[i];
  }

  Log("Writing 0x%lx at addr 0x%x\n", cpu.pc, PC_CPT_ADDR);
  auto *pc = (uint64_t *) (getPmem() + PCAddr);
  *pc = cpu.pc;

  Log("Touching 0x%x at addr 0x%x\n", CPT_MAGIC_BUMBER, BOOT_FLAGS);
  auto *flag = (uint64_t *) (getPmem() + CptFlagAddr);
  *flag = CPT_MAGIC_BUMBER;

  regDumped = true;
}

void Serializer::serialize() {
  serializeRegs();
  serializePMem();
}

void Serializer::deserialize(const char *file) {

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
}

Serializer serializer;