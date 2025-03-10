#!/bin/bash

source checkpoint_env.sh


semantic_cpt(){
  set -x
  workload=$1
  semantic_cpt_profiling_file=$2
  log=$LOG_PATH/semantic_cpt/${workload}
  mkdir -p $log
  name="semantic_cpt"
  $NEMU ${BBL_PATH}/${workload} \
      -D $RESULT -w $workload -C $name      \
      -b --checkpoint-format zstd \
      --semantic-cpt ${semantic_cpt_profiling_file} \
      > $log/${workload}-out.txt 2>${log}/${workload}-err.txt
}

export -f semantic_cpt
semantic_cpt astar_rivers
