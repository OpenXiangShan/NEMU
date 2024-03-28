#!/bin/bash

source checkpoint_env.sh

cluster(){
    set -x
    workload=$1

    export CLUSTER=$RESULT/cluster/${workload}
    mkdir -p $CLUSTER

    random1=`head -20 /dev/urandom | cksum | cut -c 1-6`
    random2=`head -20 /dev/urandom | cksum | cut -c 1-6`

    log=$LOG_PATH/cluster_logs/cluster
    mkdir -p $log

    $SIMPOINT \
        -loadFVFile $PROFILING_RES/${workload}/simpoint_bbv.gz \
        -saveSimpoints $CLUSTER/simpoints0 -saveSimpointWeights $CLUSTER/weights0 \
        -inputVectorsGzipped -maxK 30 -numInitSeeds 2 -iters 1000 -seedkm ${random1} -seedproj ${random2} \
        > $log/${workload}-out.txt 2> $log/${workload}-err.txt
}

export -f cluster

cluster bbl
