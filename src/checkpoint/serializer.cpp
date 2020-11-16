//
// Created by zyy on 2020/11/16.
//

#include "checkpoint/serializer.h"

#include <cinttypes>
#include <iostream>
#include <zlib.h>
#include <limits>

#include <memory/paddr.h>

using namespace std;

void Serializer::serializePMem() {
  uint8_t *pmem = getPmem();
  string filename = taskName + "." + phaseName + "." + weightIndicator + ".gz";
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

Serializer serializer;