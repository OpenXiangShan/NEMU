#!/bin/bash

source checkpoint_env.sh

profiling(){
    set -x
    workload=$1
    log=$LOG_PATH/profiling_logs
    mkdir -p $log

    $NEMU ${BBL_PATH}/${workload}.bin \
        -D $RESULT -w $workload -C $profiling_result_name    \
        -b --simpoint-profile --cpt-interval ${interval}            \
        -r $GCPT > $log/${workload}-out.txt 2>${log}/${workload}-err.txt
}

export -f profiling

profiling bbl
