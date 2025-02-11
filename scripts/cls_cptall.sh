#!/bin/bash

# prepare env

export NEMU_HOME=/nfs/home/cailuoshan/AISim/Sampling/NEMU
export NEMU=$NEMU_HOME/build/riscv64-nemu-interpreter
export GCPT=$NEMU_HOME/resource/gcpt_restore/build/gcpt.bin
export SIMPOINT=$NEMU_HOME/resource/simpoint/simpoint_repo/bin/simpoint

export WORKLOAD_ROOT_PATH=/nfs/home/cailuoshan/AISim/Sampling/Linux-build-sdcard/riscv-pk/build
export LOG_PATH=$NEMU_HOME/checkpoint_example_result/logs
export RESULT=$NEMU_HOME/checkpoint_example_result
export profiling_result_name=simpoint-profiling
export PROFILING_RES=$RESULT/$profiling_result_name
export interval=1  
#$((20*1000*1000))

# Build
# cd $NEMU_HOME/resource/simpoint/simpoint_repo
# make clean
# make -j
# cd $NEMU_HOME
# make clean
# make riscv64-xs-sdcard_defconfig
# make -j
# cd resource/gcpt_restore
# make clean
# make GCPT_PAYLOAD_PATH=/nfs/home/cailuoshan/AISim/Sampling/Linux-build-sdcard/riscv-pk/build/bbl.bin
# cd $NEMU_HOME


# Profiling

# profiling(){
#     set -x
#     workload=$1
#     log=$LOG_PATH/profiling_logs
#     mkdir -p $log

#     $NEMU /nfs/home/cailuoshan/AISim/Sampling/NEMU/resource/gcpt_restore/build/gcpt.bin \
#         -D $RESULT -w $workload -C $profiling_result_name    \
#         -b --simpoint-profile --cpt-interval ${interval}            \
#         > $log/${workload}-out.txt 2>${log}/${workload}-err.txt
# }

# export -f profiling

# profiling bbl


# Cluster

# cluster(){
#     set -x
#     workload=$1

#     export CLUSTER=$RESULT/cluster/${workload}
#     mkdir -p $CLUSTER

#     random1=`head -20 /dev/urandom | cksum | cut -c 1-6`
#     random2=`head -20 /dev/urandom | cksum | cut -c 1-6`

#     log=$LOG_PATH/cluster_logs/cluster
#     mkdir -p $log

#     $SIMPOINT \
#         -loadFVFile $PROFILING_RES/${workload}/simpoint_bbv.gz \
#         -saveSimpoints $CLUSTER/simpoints0 -saveSimpointWeights $CLUSTER/weights0 \
#         -inputVectorsGzipped -maxK 30 -numInitSeeds 2 -iters 1000 -seedkm ${random1} -seedproj ${random2} \
#         > $log/${workload}-out.txt 2> $log/${workload}-err.txt
# }

# export -f cluster

# cluster bbl


# Checkpointing

checkpoint(){
    set -x
    workload=$1

    export CLUSTER=$RESULT/cluster
    log=$LOG_PATH/checkpoint_logs
    mkdir -p $log
    $NEMU /nfs/home/cailuoshan/AISim/Sampling/NEMU/resource/gcpt_restore/build/gcpt.bin  \
         -D $RESULT -w ${workload} -C spec-cpt   \
         -b -S $CLUSTER --cpt-interval $interval  \
         --checkpoint-format 'bin' \
         > $log/${workload}-out.txt 2>$log/${workload}-err.txt 
}
#
export -f checkpoint

checkpoint bbl
