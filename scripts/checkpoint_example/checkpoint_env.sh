#!/bin/bash

export NEMU_HOME=
export NEMU=$NEMU_HOME/build/riscv64-nemu-interpreter
export GCPT=$NEMU_HOME/resource/gcpt_restore/build/gcpt.bin
export SIMPOINT=$NEMU_HOME/resource/simpoint/simpoint_repo/bin/simpoint

export BBL_PATH=
export LOG_PATH=$NEMU_HOME/checkpoint_example_result/logs
export RESULT=$NEMU_HOME/checkpoint_example_result
export profiling_result_name=simpoint-profiling
export PROFILING_RES=$RESULT/$profiling_result_name
export interval=$((20*1000*1000))
