#!/usr/bin/env python3

import os
import random
import sh
import operator
import os.path as osp
from os.path import join as pjoin
from os.path import expanduser as uexp
from multiprocessing import Pool

# Please add NEMU/resource/simpoint/simpoint_repo/bin into path

# Please set to the directory where gem5-generated bbvs stored
simpoint_profile_dir = '/the/mid/of/nowhere/'
simpoint_profile_dir = '/home/zyy/projects/NEMU/outputs/simpoint_profile/'

assert simpoint_profile_dir != 'deadbeaf'

def get_all():
    workloads = []
    for fname in os.listdir(simpoint_profile_dir):
        if osp.isdir(pjoin(simpoint_profile_dir, fname)) and \
                osp.isfile(pjoin(simpoint_profile_dir, fname, 'simpoint_bbv.gz')):
            workloads.append(fname)
    return workloads


def choose_weights(benchmark):
    print("choose weights for", benchmark)
    benchmark_dir = pjoin(simpoint_profile_dir, benchmark)
    os.chdir(benchmark_dir)
    max_list = []
    for i in range(10):
        weights = 'weights' + str(i)
        simpoints = 'simpoints' + str(i)

        weights_dict = {}
        cluster_dict = {}

        with open(simpoints) as sf:
            for line in sf:
                vector, n = list(map(int, line.split()))
                cluster_dict[n] = vector

        with open(weights) as wf:
            for line in wf:
                weight, n = list(map(float, line.split()))
                n = int(n)
                weights_dict[cluster_dict[n]] = weight

        sort_by_weights = sorted(iter(weights_dict.items()),
                key=operator.itemgetter(1), reverse=True)
        print(sort_by_weights)
        l = min(len(sort_by_weights), 3)
        entry = ((*sort_by_weights[:l]), sum([x for _, x in sort_by_weights[:l]]))
        print(entry)
        max_list.append(entry)

    max_list = sorted(max_list, key=operator.itemgetter(3), reverse=True)
    print(max_list)
    chosen = max_list[0][:-1]
    print(chosen)

    with open('simpoints-final', 'w') as sf:
        for i, pair in enumerate(chosen):
            sf.write(f'{pair[0]} {i}\n')
    with open('weights-final', 'w') as wf:
        for i, pair in enumerate(chosen):
            wf.write(f'{pair[1]} {i}\n')



def cluster(benchmark):
    print('cluster on', benchmark)
    simpoint_file_name = 'simpoint_bbv.gz'
    random.seed(os.urandom(8))

    benchmark_dir = pjoin(simpoint_profile_dir, benchmark)
    os.chdir(benchmark_dir)
    print(benchmark_dir)

    assert(os.path.isfile(simpoint_file_name))
    assert(os.path.isfile(pjoin(benchmark_dir, simpoint_file_name)))

    for i in range(10):
        weights = 'weights' + str(i)
        simpoints = 'simpoints' + str(i)
        # 10 times of clustering is performed
        sh.simpoint('-loadFVFile', simpoint_file_name,
                    '-saveSimpoints', simpoints,
                    '-saveSimpointWeights', weights,
                    '-inputVectorsGzipped',
                    '-maxK', 30,
                    '-numInitSeeds', 40,
                    '-iters', 100000,
                    '-seedkm', random.randint(0, 2**32 - 1),
                    '-seedproj', random.randint(0, 2**32 - 1),
                    # '-seedsample', random.randint(0, 2**32 - 1),
                   )


if __name__ == '__main__':
    p = Pool(2)
    # p.map(cluster_and_choose, get_spec())
    p.map(cluster, get_all())

