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
  .pc_cpt_addr = 0x0,
  .mode_cpt_addr = 0x8,
  .mtime_cpt_addr = 0x10,
  .mtime_cmp_cpt_addr = 0x18,
  .misc_done_cpt_addr = 0x20,
  .misc_reserve = 0x28,
  .int_reg_cpt_addr = 0x1000,
  .int_reg_done = 0x1128,
  .float_reg_cpt_addr = 0x1130,
  .float_reg_done = 0x1230,
  .csr_reg_cpt_addr = 0x1238,
  .csr_reg_done = 0x9238,
  .csr_reserve = 0x9240,
  .vector_reg_cpt_addr = 0x11240,
  .vector_reg_done = 0x13240,
};

int cpt_header_encode(void *gcpt_mmio, checkpoint_header *cpt_header, single_core_rvgc_rvv_rvh_memlayout *cpt_memlayout);

#endif
