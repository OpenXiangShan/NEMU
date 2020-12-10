import sh
import os
import os.path as osp
import sys
import argparse

from os.path import join as pjoin
from multiprocessing import Pool


# ./build/riscv64-nemu-interpreter bbl_kernel_gen/spec17_bbl/xz_cpu2006.bbl.bin
# -D outputs -w xz_cpu2006 -C simpoint_profile --simpoint-profile --interval 100000000 -b

class BatchTask:
    def __init__(self, ver):
        self.ver = ver
        self.top_out_dir = 'outputs3'
        self.task_name = f'simpoint_profile_{self.ver}'


    def run(self, t):
        workload, bbl_file, output_dir = t
        if not osp.isdir(output_dir):
            os.makedirs(output_dir)
        nemu = sh.Command('build/riscv64-nemu-interpreter')
        options = [
                bbl_file,
                '-D', self.top_out_dir,
                '-C', self.task_name,
                '-w', workload,
                '--simpoint-profile',
                '--interval', '50000000',
                '-b'
                ]
        print(options)

        sh.rm(['-f', pjoin(output_dir, 'aborted')])
        sh.rm(['-f', pjoin(output_dir, 'completed')])

        sh.touch(pjoin(output_dir, 'running'))
        try:
            nemu(
                    _out=pjoin(output_dir, 'nemu_out.txt'),
                    _err=pjoin(output_dir, 'nemu_err.txt'),

                    *options,
                    )
        except Exception as e:
            print(e)

            sh.rm(pjoin(output_dir, 'running'))
            sh.touch(pjoin(output_dir, 'aborted'))

            sys.exit(0)

        sh.rm(pjoin(output_dir, 'running'))
        sh.touch(pjoin(output_dir, 'completed'))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--spec-version', help='`06` or `17`',
            type=str, action='store', required=True)
    parser.add_argument('-w', '--workload', help='a specific workload',
            type=str, action='store', required=False)
    args = parser.parse_args()

    ver = args.spec_version
    bt = BatchTask(ver)

    bbl_dir = f'bbl_kernel_gen/spec{ver}_bbl'
    tuples = []
    for bbl in os.listdir(bbl_dir):
        if not bbl.endswith('.bin'):
            continue
        workload = bbl.split('.')[0]
        bbl_file = pjoin(bbl_dir, bbl)
        output_dir = pjoin(bt.top_out_dir, bt.task_name, workload)

        if args.workload is None or workload == args.workload:
            tuples.append((workload, bbl_file, output_dir))

    p = Pool(60)
    p.map(bt.run, tuples)


if __name__ == '__main__':
    main()

