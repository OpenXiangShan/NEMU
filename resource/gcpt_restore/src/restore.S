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
  .section ".restore.code","ax",@progbits
  .globl boot_decide_vector

boot_decide_vector:
  li s0, BOOT_FLAG_ADDR
  ld t1, (s0) #load boot flag
  li t2, CPT_MAGIC_BUMBER
  beq t1, t2, restore_csr_vector
  j payload_bin

mode_bad:
  li a0, RESTORE_MODE_BAD
  j restore_trap_handler

restore_trap_handler:
  .insn r 0x6B, 0, 0, x0, x0, x0

restore_csr_vector:

  li t0, CSR_REG_CPT_ADDR
  CSRS(CSRS_RESTORE)
  HCSRS(CSRS_RESTORE)
  VCSRS(CSRS_RESTORE)

  li t0, MSTATUS_MPP # MPP is M
  li s2, MODE_CPT_ADDR
  ld s2, (s2) #load mode flag into s2
  slli t1, s2, 11 # mode flag shift to MPP
  beq t0, t1, mode_bad #bad trap if mode flag is M

  # set mtime (inaccurate) and mtimecmp
#  li t0, CLINT_MMIO+CLINT_MTIMECMP
#  li t1, MTIME_CMP_CPT_ADDR
#  ld t1, (t1)
#  sd t1, (t0)
##
#  li t0, CLINT_MMIO+CLINT_MTIME
#  li t1, MTIME_CPT_ADDR
#  ld t1, (t1)
#  sd t1, (t0)

 # li t0, MIE_MTIE
 # csrs CSR_MIE, t0

  # csrr t1, CSR_MSTATUS
  # srli t0, t1, 63
  # beqz t0, restore_pc_vector

restore_float_vector:
  # set fs
  li t0, MSTATUS_FS
  csrs  CSR_MSTATUS, t0

  li sp, FLOAT_REG_CPT_ADDR # load float section addr

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

  .section ".payload", "ax", %progbits
  .align 4
  .global payload_bin
payload_bin:
#ifndef FW_PAYLOAD_PATH
	wfi
	j	payload_bin
#else
	.incbin	FW_PAYLOAD_PATH
#endif
