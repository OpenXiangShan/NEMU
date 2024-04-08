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

#ifndef __RESTORE_ROM_ADDR__
#define __RESTORE_ROM_ADDR__

#define CPT_MAGIC_BUMBER        0xbeef
#define BOOT_CODE               0x80000000

#define BOOT_FLAG_ADDR          0x800ECDB0
#define PC_CPT_ADDR             0x800ECDB8
#define MODE_CPT_ADDR           0x800ECDC0
#define MTIME_CPT_ADDR          0x800ECDC8
#define MTIME_CMP_CPT_ADDR      0x800ECDD0
#define MISC_DONE_CPT_ADDR      0x800ECDD8
#define MISC_RESERVE            0x800ECDE0

#define INT_REG_CPT_ADDR        0x800EDDE0
#define INT_REG_DONE            0x800EDEE0

#define FLOAT_REG_CPT_ADDR      0x800EDEE8
#define FLOAT_REG_DONE          0x800EDFE8

#define CSR_REG_CPT_ADDR        0x800EDFF0
#define CSR_REG_DONE            0x800F5FF0
#define CSR_RESERVE             0x800F5FF8

#define VECTOR_REG_CPT_ADDR     0x800FDFF8
#define VECTOR_REG_DONE         0x800FFFF8

#define GCPT_CHECKPOINT_VERSION 0x800FFFFC

#ifndef RESET_VECTOR
    #define RESET_VECTOR        0x80100000
#endif

#define CLINT_MMIO              0x38000000
#define CLINT_MTIMECMP          0x4000
#define CLINT_MTIME             0xBFF8

#define RESTORE_GOOD            0x0
#define RESTORE_MODE_BAD        0x1
#define GCPT_INCOMPLETE         0x2
#define VERSION_NOT_MATCH       0x3

#define COMPLETE_FLAG           0xcaff
#define GCPT_VERSION            0x20231222

#define MISA_V 0x200000
#define MISA_H 0x80

#endif //__RESTORE_ROM_ADDR__
