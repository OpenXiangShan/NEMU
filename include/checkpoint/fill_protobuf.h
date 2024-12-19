#ifndef __FILL_PROTOBUF_H__
#define __FILL_PROTOBUF_H__

#include "checkpoint.pb.h"

#define MAGIC_NUMBER 0xdeadbeef
__attribute__((unused))
static checkpoint_header default_cpt_header = {
  .magic_number = MAGIC_NUMBER,
  .cpt_offset = sizeof(checkpoint_header) + sizeof(single_core_rvgc_rvv_rvh_memlayout),
  .cpu_num = 1,
  .single_core_size = 1 * 1024 * 1024,
  .version = 0x20240125,
};

__attribute__((unused))
static single_core_rvgc_rvv_rvh_memlayout default_cpt_percpu_layout = {
  .pc_cpt_addr         = 0xECDB8,
  .mode_cpt_addr       = 0xECDC0,
  .mtime_cpt_addr      = 0xECDC8,
  .mtime_cmp_cpt_addr  = 0xECDD0,
  .misc_done_cpt_addr  = 0xECDD8,
  .misc_reserve        = 0xECDE0,
  .int_reg_cpt_addr    = 0xEDDE0,
  .int_reg_done        = 0xEDEE0,
  .float_reg_cpt_addr  = 0xEDEE8,
  .float_reg_done      = 0xEDFE8,
  .csr_reg_cpt_addr    = 0xEDFF0,
  .csr_reg_done        = 0xF5FF0,
  .csr_reserve         = 0xF5FF8,
  .vector_reg_cpt_addr = 0xFDFF8,
  .vector_reg_done     = 0xFFFF8,
};

int cpt_header_encode(void *gcpt_mmio, checkpoint_header *cpt_header, single_core_rvgc_rvv_rvh_memlayout *cpt_memlayout);

#endif
