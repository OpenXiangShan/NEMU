#!/bin/bash

source env.sh

# Profiling

profiling(){
    set -x
    workload=$1
    log=$LOG_PATH/profiling_logs
    mkdir -p $log

    $NEMU ${WORKLOAD_ROOT_PATH}/${workload}.bin \
        -D $RESULT -w $workload -C $profiling_result_name    \
        -b --simpoint-profile --cpt-interval ${interval}            \
        -r $GCPT > $log/${workload}-out.txt 2>${log}/${workload}-err.txt
}

export -f profiling

profiling bbl