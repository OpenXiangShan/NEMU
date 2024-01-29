#!/bin/bash

export NEMU_HOME=
export BBL_PATH=
export CLUSTER_PATH=
export LOG_PATH=
export NEMU=$NEMU_HOME/build/riscv64-nemu-interpreter
export GCPT=$NEMU_HOME/resource/gcpt_restore/build/gcpt.bin

export interval=$((20*1000*1000))

checkpoint(){
    set -x
    workload=$1
    CLUSTER=$RESULT/cluster

    log=$LOG_PATH/checkpoint_logs
    mkdir -p $log
    $NEMU ${BBL_PATH}/${workload}-bbl-linux-spec.bin \
         -D $RESULT -w ${workload} -C spec-cpt${cluster_times}         \
         -b -S $CLUSTER --cpt-interval $interval \
         -r $GCPT > $log/${workload}-out.txt 2>$log/${workload}-err.txt 
}
