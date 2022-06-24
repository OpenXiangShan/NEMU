#***************************************************************************************
# Copyright (c) 2014-2021 Zihao Yu, Nanjing University
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#**************************************************************************************/

#!/usr/bin/env python3

from utils import export

WHITE_LIST = [
  r'.gitignore',
  r'^/*$',
  r'Makefile',
  r'^/src/',
  r'^/include/',
  r'^/tools/gen-expr/',
  r'^/tools/kvm-diff/',
  r'^/tools/qemu-diff/',
]

BLACK_LIST = [
  r'/build/',
  r'/export/',
  r'/.git/',
  r'riscv64',
  r'/difftest/ref.c',
  r'^/resource/',
  r'^/src/device/sdcard.c',
  r'^/src/device/mmc.h',
  r'^/src/engine/rv64',
  r'^/src/isa/mips32/local-include/intr.h',
  r'^/src/isa/x86/exec/eflags.c',
  r'^/src/isa/x86/exec/lazycc.h',
  r'^/src/isa/x86/exec/string.h',
  r'^/src/isa/x86/kvm/',
  r'^/src/isa/x86/local-include/mmu.h',
  r'^/src/memory/vaddr.c',
]

export(WHITE_LIST, BLACK_LIST)
