#!/usr/bin/env python3
import subprocess
import os
import time
import signal
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--cluster", type=str, required=True)
parser.add_argument('--cpt-interval', type=int, default=100000000)
parser.add_argument('-w', '--workload', type=str, required=True)
parser.add_argument('-b', '--bbl', type=str, required=True)
parser.add_argument("--nemu-home", type=str, default='/nfs/home/zhangchuanqi/lvna/for_xs/xs-env/nemu-tracing')
parser.add_argument('--simpoint-cluster-dir', type=str, default='/nfs/home/share/zhangchuanqi/gcpt_cluster')
parser.add_argument('--cpts-dir', type=str, default='/nfs/home/share/zhangchuanqi/gcpt_cpts')

args = parser.parse_args()

os.environ['NEMU_HOME'] = args.nemu_home
os.chdir(args.nemu_home)


generate_bbv_cmd = f'./build/riscv64-nemu-interpreter -b -D outputs -C {args.cluster} ' + \
f'--cpt-interval {args.cpt_interval} ' + \
'-r resource/gcpt_restore/build/gcpt.bin ' + \
f'{args.bbl} ' + \
f'-w {args.workload} ' + \
'--simpoint-profile '

print(generate_bbv_cmd)
# subprocess.run(generate_bbv_cmd, shell=True)

real_cluster_dir = os.path.join(args.simpoint_cluster_dir, args.cluster)
real_workload_simpoint_dir = os.path.join(args.simpoint_cluster_dir, args.cluster, args.workload)
os.makedirs(real_workload_simpoint_dir, exist_ok=True)

generate_weights_cmd = f'./resource/simpoint/simpoint_repo/bin/simpoint \
-loadFVFile ./outputs/{args.cluster}/{args.workload}/simpoint_bbv.gz \
-saveSimpoints {real_workload_simpoint_dir}/simpoints0 -saveSimpointWeights {real_workload_simpoint_dir}/weights0 \
-inputVectorsGzipped -maxK 30 -numInitSeeds 2 -iters 1000 -seedkm 123456 -seedproj 654321'


print(generate_weights_cmd)
# subprocess.run(generate_weights_cmd, shell=True)

take_cpt_cmd = f'./build/riscv64-nemu-interpreter {args.bbl} \
-D {args.cpts_dir} -w {args.workload} -C {args.cluster} \
-b -S {real_cluster_dir} --cpt-interval {args.cpt_interval} \
-r resource/gcpt_restore/build/gcpt.bin'

print(take_cpt_cmd)
subprocess.run(take_cpt_cmd, shell=True)