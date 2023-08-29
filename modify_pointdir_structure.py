#!/usr/bin/env python3
import subprocess
import os
import time
import signal
import argparse
import shutil

parser = argparse.ArgumentParser()
parser.add_argument('-d', '--cpts-dir', type=str, required=True)
args = parser.parse_args()
base_dir = args.cpts_dir

# base_dir = '/nfs/home/share/zhangchuanqi/gcpt_cpts/simpoint-spec06-gcb-o2-50M'

os.chdir(base_dir)

workloads = os.listdir(base_dir)
workloads = list(filter(lambda x: len(x.split('_'))<=2, workloads))

print(workloads)

for w in workloads:
    workload_dir = os.path.join(base_dir, w)
    cpt_idxs = os.listdir(workload_dir)
    for cidx in cpt_idxs:
        if not cidx.isdigit():
            continue
        cpt_dir = os.path.join(workload_dir, cidx)
        cpt_files = os.listdir(cpt_dir)
        for cpt_file in cpt_files:
            if cpt_file.endswith('gz'):
                try:
                    weight = float(cpt_file.split('_')[-2])
                except:
                    weight = 0.0
                if weight < 0.000001:
                    continue
                cpt_path = os.path.join(cpt_dir, cpt_file)
                new_cpt_dirname = f'{w}{cpt_file[:-4]}'
                key = cpt_file.split('_')[1]
                new_cpt_dir = os.path.join(base_dir, new_cpt_dirname, '0')
                os.makedirs(new_cpt_dir, exist_ok=True)
                new_cpt_path = os.path.join(new_cpt_dir, f'_{key}_.gz')
                # print(f'mv {cpt_path} {new_cpt_path}')
                shutil.move(cpt_path, new_cpt_path)
    print(f'rm -rf {workload_dir}')
    shutil.rmtree(workload_dir)
