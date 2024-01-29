#!/bin/bash

export NEMU_HOME=
export BBL_PATH=
export LOG_PATH=
export RESULT=
export NEMU=$NEMU_HOME/build/riscv64-nemu-interpreter
export profiling_result_name=simpoint-profiling
export PROFILING_RES=$RESULT/$profiling_result_name
export GCPT=$NEMU_HOME/resource/gcpt_restore/build/gcpt.bin

export interval=$((20*1000*1000))

profiling(){
    set -x
    workload=$1
    log=$LOG_PATH/profiling_logs
    mkdir -p $log

    $NEMU ${BBL_PATH}/${workload}-bbl-linux-spec.bin \
        -D $RESULT -w $workload -C $profiling_result_name    \
        -b --simpoint-profile --cpt-interval ${interval}            \
        -r $GCPT > $log/${workload}-out.txt 2>${log}/${workload}-err.txt
}


