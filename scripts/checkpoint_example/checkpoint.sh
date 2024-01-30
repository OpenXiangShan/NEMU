#!/bin/bash

source checkpoint_env.sh

checkpoint(){
    set -x
    workload=$1

    export CLUSTER=$RESULT/cluster
    log=$LOG_PATH/checkpoint_logs
    mkdir -p $log
    $NEMU ${BBL_PATH}/${workload}.bin \
         -D $RESULT -w ${workload} -C spec-cpt${cluster_times}         \
         -b -S $CLUSTER --cpt-interval $interval \
         -r $GCPT > $log/${workload}-out.txt 2>$log/${workload}-err.txt 
}

export -f checkpoint

checkpoint bbl
