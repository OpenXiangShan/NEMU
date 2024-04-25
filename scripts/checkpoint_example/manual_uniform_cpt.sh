#!/bin/bash

source checkpoint_env.sh

# cannot generate checkpoint when exec in shell script
manual_uniform_cpt(){
    set -x
    workload=$1
    log=$LOG_PATH/manual_uniform
    mkdir -p $log
    name="manual_uniform"

    $NEMU ${BBL_PATH}/${workload}-bbl-linux-spec.bin \
        -D $RESULT -w $workload -C $name      \
        -b --cpt-interval ${interval}            \
        --manual-uniform-cpt \
        -r $GCPT > $log/${workload}-out.txt 2>${log}/${workload}-err.txt

}
