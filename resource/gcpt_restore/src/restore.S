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
  li s0, BOOT_FLAGS  
  ld t1, (s0) #load boot flag
  li t2, CPT_MAGIC_BUMBER
  beq t1, t2,1f
  j RESET_VECTOR

1:
restore_csr_vector:
  li t0, CSR_CPT_ADDR
  CSRS(CSRS_RESTORE)

  # set MPIE to the value of MIE
  csrr t0, CSR_MSTATUS
  li t1, MSTATUS_MIE
  and t2, t1, t0
  slli t2, t2, 4 # shift to MPIE
  csrs CSR_MSTATUS, t2

  # clear MIE
  csrc CSR_MSTATUS, t1


  li t0, MSTATUS_MPP # MPP is M
  ld s2, 8(s0) #load mode flag into s2
  slli t1, s2, 11 # mode flag shift to MPP
2:
  beq t0, t1, 2b #stuck if mode flag is M
  # set MPP to mode flag (U or S), clear and set
  csrc CSR_MSTATUS, t0
  csrs CSR_MSTATUS, t1

  # set fs
  li t0, MSTATUS_FS
  csrs  CSR_MSTATUS, t0

  # set mtime (inaccurate) and mtimecmp
  li t0, CLINT_MMIO+CLINT_MTIMECMP
  ld t1, 24(s0)
  sd t1, (t0)

  li t0, CLINT_MMIO+CLINT_MTIME
  ld t1, 16(s0)
  sd t1, (t0)

  li t0, MIE_MTIE
  csrs CSR_MIE, t0

  # csrr t1, CSR_MSTATUS
  # srli t0, t1, 63
  # beqz t0, restore_pc_vector

restore_float_vector:
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

restore_pc_vector:
  li t0, PC_CPT_ADDR
  ld t0, (t0)
  csrw CSR_MEPC, t0

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
  .section ".restore.flag","a",@progbits
  .word 0xdead

  .align 3
  .section ".restore.data","a",@progbits
  .8byte 0x0
  .8byte 0x1
  .8byte 0x2
  .8byte 0x3
  .8byte 0x4
  .8byte 0x5
  .8byte 0x6
  .8byte 0x7
  .8byte 0x8
  .8byte 0x9
  .8byte 0xa
  .8byte 0xb
  .8byte 0xc
  .8byte 0xd
  .8byte 0xe
  .8byte 0xf
  .8byte 0x10
  .8byte 0x11
  .8byte 0x12
  .8byte 0x13
  .8byte 0x14
  .8byte 0x15
  .8byte 0x16
  .8byte 0x17
  .8byte 0x18
  .8byte 0x19
  .8byte 0x1a
  .8byte 0x1b
  .8byte 0x1c
  .8byte 0x1d
  .8byte 0x1e
  .8byte 0x1f
  
