#!/usr/bin/env python3

from utils import export

WHITE_LIST = [
  r'.gitignore',
  r'riscv64',
  r'^/*$',
  r'Makefile',
  r'^/src/*$',
  r'^/src/monitor/*$',
  r'^/src/monitor/debug/*$',
  r'^/src/monitor/difftest/*$',
  r'^/src/device/*$',
  r'^/src/device/io/*$',
  r'^/src/engine/interpreter/*$',
  r'^/src/memory/*$',
  r'^/include/*$',
  r'^/include/cpu/*$',
  r'^/include/device/*$',
  r'^/include/memory/*$',
  r'^/include/monitor/*$',
  r'^/include/rtl/*$',
]

BLACK_LIST = [
  r'/build/',
  r'/export/',
  r'/gen-expr/',
  r'/kvm-diff/',
  r'/recorder/',
  r'/.git/',
  r'mips32',
  r'riscv32',
  r'x86',
  r'/resource/bbl',
  r'/engine/rv64',
  r'/device/audio.c',
  r'/device/vga.c',
  r'/device/keyboard.c',
  r'runall.sh',
]

export(WHITE_LIST, BLACK_LIST)
