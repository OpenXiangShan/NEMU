#!/bin/bash

source checkpoint_env.sh

uniform_cpt(){
    set -x
    workload=$1
    log=$LOG_PATH/uniform
    mkdir -p $log
    name="uniform"

    $NEMU ${BBL_PATH}/${workload}.bin \
        -D $RESULT -w $workload -C $name      \
        -b -u --cpt-interval ${interval}   --dont-skip-boot         \
        -r $GCPT > $log/${workload}-out.txt 2>${log}/${workload}-err.txt
}

export -f uniform_cpt

uniform_cpt bbl
