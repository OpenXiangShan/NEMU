#!/bin/bash

export NEMU_HOME=
export BBL_PATH=
export LOG_PATH=
export RESULT=
export NEMU=$NEMU_HOME/build/riscv64-nemu-interpreter
export GCPT=$NEMU_HOME/resource/gcpt_restore/build/gcpt.bin

export interval=$((20*1000*1000))

manual_oneshot_cpt(){
    set -x
    workload=$1
    log=$LOG_PATH/manual_oneshot/${workload}
    mkdir -p $log

    name="manual_oneshot"

    $NEMU ${BBL_PATH}/${workload}-bbl-linux-spec.bin \
        -D $RESULT -w $workload -C $name      \
        -b --cpt-interval ${interval}            \
        --manual-oneshot-cpt \
        -r $GCPT > $log/${workload}-out.txt 2>${log}/${workload}-err.txt

}
