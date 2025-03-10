#!/bin/bash

source checkpoint_env.sh


sematic_cpt(){
  set -x
  workload=$1
  sematic_cpt_profiling_file=$2
  log=$LOG_PATH/sematic_cpt/${workload}
  mkdir -p $log
  name="sematic_cpt"
  $NEMU ${BBL_PATH}/${workload} \
      -D $RESULT -w $workload -C $name      \
      -b --checkpoint-format zstd \
      --sematic-cpt ${sematic_cpt_profiling_file} \
      > $log/${workload}-out.txt 2>${log}/${workload}-err.txt
}

export -f sematic_cpt
sematic_cpt astar_rivers
