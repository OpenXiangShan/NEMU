#!/bin/bash

# prepare env

set -x

export NEMU_HOME=/home/xiongtianyu/xs/NEMU
export NEMU=$NEMU_HOME/build/riscv64-nemu-interpreter
export GCPT=$NEMU_HOME/resource/gcpt_restore/build/gcpt.bin
export SIMPOINT=$NEMU_HOME/resource/simpoint/simpoint_repo/bin/simpoint

export WORKLOAD_ROOT_PATH=/home/xiongtianyu/workloads/bins/bbl-gcc-jeMalloc
export RESULT=$NEMU_HOME/parallel_result
export LOG_PATH=$RESULT/logs
export profiling_result_name=profiling
export PROFILING_RES=$RESULT/$profiling_result_name
export interval=$((20*1000*1000))


# Profiling
profiling(){
    set -x
    workload=$1
    log=$LOG_PATH/profiling_logs/${workload}
    mkdir -p $log

    $NEMU ${WORKLOAD_ROOT_PATH}/${workload}-bbl-linux-spec.bin \
        -D $RESULT -w $workload -C $profiling_result_name    \
        -b --simpoint-profile --cpt-interval ${interval}            \
        -r $GCPT > $log/${workload}-out.txt 2>${log}/${workload}-err.txt
}
export -f profiling


# Cluster
cluster(){
    set -x
    workload=$1

    export CLUSTER=$RESULT/cluster/${workload}
    mkdir -p $CLUSTER

    random1=`head -20 /dev/urandom | cksum | cut -c 1-6`
    random2=`head -20 /dev/urandom | cksum | cut -c 1-6`

    log=$LOG_PATH/cluster_logs/${workload}
    mkdir -p $log

    $SIMPOINT \
        -loadFVFile $PROFILING_RES/${workload}/simpoint_bbv.gz \
        -saveSimpoints $CLUSTER/simpoints0 -saveSimpointWeights $CLUSTER/weights0 \
        -inputVectorsGzipped -maxK 30 -numInitSeeds 2 -iters 1000 -seedkm ${random1} -seedproj ${random2} \
        > $log/${workload}-out.txt 2> $log/${workload}-err.txt
}
export -f cluster


# Checkpointing
checkpoint(){
    set -x
    workload=$1

    export CLUSTER=$RESULT/cluster/
    log=$LOG_PATH/checkpoint_logs/${workload}/raw
    mkdir -p $log
    $NEMU ${WORKLOAD_ROOT_PATH}/${workload}-bbl-linux-spec.bin  \
         -D $RESULT -w ${workload} -C checkpoint   \
         -b -S $CLUSTER --cpt-interval $interval \
         --checkpoint-format raw \
         -r $GCPT > $log/${workload}-out.txt 2>$log/${workload}-err.txt 
}
export -f checkpoint


# Restore
restore(){
    set -x
    all_args=("$@")
    args=(${all_args[0]})
    workload=${args[0]}
    ckpt=${args[1]}

    log=$LOG_PATH/restore_logs/${workload}/raw
    mkdir -p $log
    $NEMU --restore ${NEMU_HOME}/${ckpt} \
          -b  -I ${interval} \
          > $log/${workload}-out.txt 2>$log/${workload}-err.txt 
}
export -f restore

# Run
run(){
    set -x
    workload=$1
    log=$LOG_PATH/run_logs/${workload}/on
    mkdir -p $log

    $NEMU ${WORKLOAD_ROOT_PATH}/${workload}-bbl-linux-spec.bin \
        -b -x  -r $GCPT > $log/${workload}-out.txt 2>${log}/${workload}-err.txt    
}
export -f run


export workload_list=$NEMU_HOME/scripts/simpoint/workload_list.txt
export checkpoint_list=$NEMU_HOME/scripts/simpoint/checkpoint_list.txt

parallel_profiling(){
	export num_threads=20
    cat $workload_list | parallel -a - -j $num_threads profiling {}
}
export -f parallel_profiling

parallel_cluster(){
    export num_threads=20
    cat $workload_list | parallel -a - -j $num_threads time cluster {}
}
export -f parallel_cluster

parallel_checkpoint(){
    export num_threads=20
    cat $workload_list | parallel -a - -j $num_threads checkpoint {}
}
export -f parallel_checkpoint

parallel_restore(){
    export num_threads=20
    cat $checkpoint_list | parallel -a - -j $num_threads  restore {}
}
export -f parallel_restore

parallel_run(){
    export num_threads=20
    cat $workload_list | parallel -a - -j $num_threads run {}
}
export -f parallel_run

# parallel_profiling

# parallel_cluster

# parallel_checkpoint

# parallel_restore

# parallel_run