/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include <common.h>

#define _str(x) # x
#define str(x) _str(x)

char *isa_qemu_argv[] = {
  "/usr/bin/qemu-system-mipsel",
  "-nographic", "-S", "-serial", "none", "-monitor", "none",
  "-machine", "mipssim", "-kernel", str(NEMU_HOME) "/resource/mips-elf/mips.dummy",
  NULL
};

void init_isa() {
}
