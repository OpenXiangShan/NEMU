#!/bin/bash

source checkpoint_env.sh



# can not generate checkpoint when exec in shell script
manual_oneshot_cpt(){
    set -x
    workload=$1
    log=$LOG_PATH/manual_oneshot/${workload}
    mkdir -p $log

    name="manual_oneshot"

    $NEMU ${BBL_PATH}/${workload}-bbl-linux-spec.bin \
        -D $RESULT -w $workload -C $name      \
        -b --cpt-interval ${interval}            \
        --manual-oneshot-cpt --checkpoint-format zstd > $log/${workload}-out.txt 2>${log}/${workload}-err.txt

}
