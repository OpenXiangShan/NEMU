#!/bin/bash

# prepare env

set -x

export NEMU_HOME=/home/xiongtianyu/xs/NEMU
export NEMU=$NEMU_HOME/build/riscv64-nemu-interpreter
export GCPT=$NEMU_HOME/resource/gcpt_restore/build/gcpt.bin
export SIMPOINT=$NEMU_HOME/resource/simpoint/simpoint_repo/bin/simpoint

export WORKLOAD_ROOT_PATH=/home/xiongtianyu/workloads/bins
export LOG_PATH=$NEMU_HOME/simpoint_result/logs
export RESULT=$NEMU_HOME/simpoint_result
export profiling_result_name=profiling
export PROFILING_RES=$RESULT/$profiling_result_name
export interval=$((20*1000*1000))
