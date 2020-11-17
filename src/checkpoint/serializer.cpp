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

using namespace std;

Serializer::Serializer() :
    IntRegStartAddr(0x10000),
    FloatRegStartAddr(0x10100),
    CSRStartAddr(0x10200) // ends at 0x11200
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
}

extern void csr_writeback();

void Serializer::serializeRegs() {
  auto *intRegCpt = (uint64_t *) (getPmem() + IntRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(intRegCpt + i) = cpu.gpr[i]._64;
  }

  auto *floatRegCpt = (uint64_t *) (getPmem() + FloatRegStartAddr);
  for (unsigned i = 0; i < 32; i++) {
    *(floatRegCpt + i) = cpu.fpr[i]._64;
  }

  csr_writeback();
  auto *csrCpt = (uint64_t *) (getPmem() + CSRStartAddr);
  for (unsigned i = 0; i < 4096; i++) {
    *(csrCpt + i) = csr_array[i];
  }

  regDumped = true;
}

Serializer serializer;