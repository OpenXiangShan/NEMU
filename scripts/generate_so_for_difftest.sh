#!/usr/bin/env bash

set -x
set -v

export NEMU_HOME=/root/NEMU

# gcpt_restore is not needed for ref-so. Just skip it in Makefile.
mkdir -p $(pwd)/resource/gcpt_restore/src
touch $(pwd)/resource/gcpt_restore/src/restore_rom_addr.h
mkdir -p $(pwd)/resource/gcpt_restore/build
touch $(pwd)/resource/gcpt_restore/build/gcpt.bin

artifact_dir=$(pwd)/artifact
mkdir -p $artifact_dir
make clean
make riscv64-xs-ref_defconfig
make -j
cp build/riscv64-nemu-interpreter-so ${artifact_dir}/riscv64-nemu-interpreter-so

make clean
make riscv64-xs-ref-debug_defconfig
make -j
cp build/riscv64-nemu-interpreter-so ${artifact_dir}/riscv64-nemu-interpreter-debug-so

make clean
make riscv64-dual-xs-ref_defconfig
make -j
cp build/riscv64-nemu-interpreter-so ${artifact_dir}/riscv64-nemu-interpreter-dual-so

make clean
make riscv64-dual-xs-ref-debug_defconfig
make -j
cp build/riscv64-nemu-interpreter-so ${artifact_dir}/riscv64-nemu-interpreter-dual-debug-so

make clean
make riscv64-xs-ref_bitmap_defconfig
make -j
cp build/riscv64-nemu-interpreter-so ${artifact_dir}/riscv64-nemu-interpreter-bitmap-so
