#include <common.h>
#include "sdi.h"

static rtlreg_t spm[NR_SPM] = {};

static inline rtlreg_t sdi_spm_read(uint32_t funct3) {
  assert(funct3 < NR_SPM);
  return spm[funct3];
}

static inline rtlreg_t sdi_spm_write(uint32_t funct3, rtlreg_t rs1) {
  assert(funct3 < NR_SPM);
  spm[funct3] = rs1;
  return 0;
}

rtlreg_t rocc_sdi(uint32_t funct7, uint32_t funct3, rtlreg_t rs1, rtlreg_t rs2) {
  switch (funct7) {
    case SDI_ROCC_SPM_READ: return sdi_spm_read(funct3);
    case SDI_ROCC_SPM_WRITE: return sdi_spm_write(funct3, rs1);
    default: assert(0);
  }
}
