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

#include "restore_rom_addr.h"
#include "encoding.h"
#include "csr.h"



  .option norvc

  .section ".restore.size","ax",@progbits
  .globl place_of_restore_size
place_of_restore_size:
  j boot_decide_vector
  .word gcpt_end-gcpt_begin

  .section ".restore.code","ax",@progbits
  .globl boot_decide_vector
boot_decide_vector:
  la s0, read_complete_flag #load read complete flag address
  ld t1, (s0)
  li t2, COMPLETE_FLAG
  bne t1, t2, read_incomplete # read incomplete goto bad_trap

  li s0, BOOT_FLAG_ADDR
  ld t1, (s0) #load boot flag
  li t2, CPT_MAGIC_BUMBER
  beq t1, t2, restore_csr_vector
  j payload_bin # not found restore flag, goto payload

#version_not_match:
#  li a0, VERSION_NOT_MATCH
#  j restore_trap_handler

read_incomplete:
  li a0, GCPT_INCOMPLETE
  j restore_trap_handler

mode_bad:
  li a0, RESTORE_MODE_BAD
  j restore_trap_handler

restore_trap_handler:
  .insn r 0x6B, 0, 0, x0, x0, x0

#gcp_checkpoint_version_check:
#  li t0, GCPT_CHECKPOINT_VERSION
#  ld t1, (t0)
#  li t0, GCPT_VERSION
#  bne t1, t0, version_not_match
#
restore_csr_vector:
  li t0, CSR_REG_CPT_ADDR
  li t2, 0x003
  slli t2, t2, 3
  add t2, t0, t2
  ld t1, (t2)
  li t0, MSTATUS_FS
  csrs  CSR_MSTATUS, t0
  csrw 0x003, t1

  li t0, CSR_REG_CPT_ADDR
  CSRS(CSRS_RESTORE)

hext_csr_restore:
  csrr t0, misa
  li t1, MISA_H
  and t0, t0, t1
  bne t0,t1,vext_restore
  li t0, CSR_REG_CPT_ADDR
  HCSRS(CSRS_RESTORE)

vext_restore:
  csrr t0, misa
  li t1, MISA_V
  and t0, t0, t1
  bne t0,t1,normal_csr_restore
  RESTORE_VECTORS(CSRS_RESTORE)
  li t0, CSR_REG_CPT_ADDR
  VCSRS(CSRS_RESTORE)

normal_csr_restore:
  li t0, MSTATUS_MPP # MPP is M
  li s2, MODE_CPT_ADDR
  ld s2, (s2) #load mode flag into s2
  slli t1, s2, 11 # mode flag shift to MPP
  beq t0, t1, mode_bad #bad trap if mode flag is M

  # set mtime (inaccurate) and mtimecmp
  li t0, CLINT_MMIO+CLINT_MTIMECMP
  li t1, MTIME_CMP_CPT_ADDR
  ld t1, (t1)
  sd t1, (t0)
###
  li t0, CLINT_MMIO+CLINT_MTIME
  li t1, MTIME_CPT_ADDR
  ld t1, (t1)
  sd t1, (t0)

  # li t0, MIE_MTIE
  # csrs CSR_MIE, t0

  # csrr t1, CSR_MSTATUS
  # srli t0, t1, 63
  # beqz t0, restore_pc_vector

restore_float_vector:
  li sp, FLOAT_REG_CPT_ADDR # load float section addr

  # set fs
  li t0, MSTATUS_FS
  csrs  CSR_MSTATUS, t0

  fld f0, (0*8)(sp)
  fld f1, (1*8)(sp)
  fld f2, (2*8)(sp)
  fld f3, (3*8)(sp)
  fld f4, (4*8)(sp)
  fld f5, (5*8)(sp)
  fld f6, (6*8)(sp)
  fld f7, (7*8)(sp)
  fld f8, (8*8)(sp)
  fld f9, (9*8)(sp)
  fld f10, (10*8)(sp)
  fld f11, (11*8)(sp)
  fld f12, (12*8)(sp)
  fld f13, (13*8)(sp)
  fld f14, (14*8)(sp)
  fld f15, (15*8)(sp)
  fld f16, (16*8)(sp)
  fld f17, (17*8)(sp)
  fld f18, (18*8)(sp)
  fld f19, (19*8)(sp)
  fld f20, (20*8)(sp)
  fld f21, (21*8)(sp)
  fld f22, (22*8)(sp)
  fld f23, (23*8)(sp)
  fld f24, (24*8)(sp)
  fld f25, (25*8)(sp)
  fld f26, (26*8)(sp)
  fld f27, (27*8)(sp)
  fld f28, (28*8)(sp)
  fld f29, (29*8)(sp)
  fld f30, (30*8)(sp)
  fld f31, (31*8)(sp)

restore_int_vector:
  li sp, INT_REG_CPT_ADDR # load int section addr
  ld x1, (1*8)(sp)
  ld x3, (3*8)(sp)
  ld x4, (4*8)(sp)
  ld x5, (5*8)(sp)
  ld x6, (6*8)(sp)
  ld x7, (7*8)(sp)
  ld x8, (8*8)(sp)
  ld x9, (9*8)(sp)
  ld x10, (10*8)(sp)
  ld x11, (11*8)(sp)
  ld x12, (12*8)(sp)
  ld x13, (13*8)(sp)
  ld x14, (14*8)(sp)
  ld x15, (15*8)(sp)
  ld x16, (16*8)(sp)
  ld x17, (17*8)(sp)
  ld x18, (18*8)(sp)
  ld x19, (19*8)(sp)
  ld x20, (20*8)(sp)
  ld x21, (21*8)(sp)
  ld x22, (22*8)(sp)
  ld x23, (23*8)(sp)
  ld x24, (24*8)(sp)
  ld x25, (25*8)(sp)
  ld x26, (26*8)(sp)
  ld x27, (27*8)(sp)
  ld x28, (28*8)(sp)
  ld x29, (29*8)(sp)
  ld x30, (30*8)(sp)
  ld x31, (31*8)(sp)
  ld sp, (2*8)(sp)
  mret

  .align 3
read_complete_flag:
  .word COMPLETE_FLAG
  .word 0x0

  .section ".payload", "ax", %progbits
  .align 4
  .global payload_bin
payload_bin:
#ifndef GCPT_PAYLOAD_PATH
  wfi
  j payload_bin
#else
  .incbin GCPT_PAYLOAD_PATH
#endif
