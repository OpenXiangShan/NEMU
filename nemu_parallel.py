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
    def __init__(self, ver, taskname):
        self.ver = ver
        self.source_data_dir = 'outputs3'
        self.top_out_dir = '/home51/zyy/expri_results'
        self.task = taskname
        self.task_name = taskname + f'_{self.ver}'

        self.simpoint_profile_dir = None
        if taskname == 'take_simpoint_checkpoint':
            self.simpoint_profile_dir = f'{self.source_data_dir}/simpoint_profile_{self.ver}'

        self.task_options = {
        'simpoint_profile': [
            '--interval', '50000000',
            '-D', self.top_out_dir,
            '--simpoint-profile',
            ],

        'betapoint_profile': [
            '--interval', '10000',
            '-D', self.top_out_dir,
            '--betapoint-profile',
            ],

        'take_simpoint_checkpoint': [
            '--interval', '50000000',
            '-D', self.top_out_dir,
            '-S', self.simpoint_profile_dir,
            ],
        }
        self.extra_options = self.task_options[taskname]

    def check_prereq(self, w):
        if self.task == 'simpoint_profile':
            return True

        if self.task == 'betapoint_profile':
            return True

        if self.task == 'take_simpoint_checkpoint':
            flag = True
            if not osp.isfile(pjoin(self.simpoint_profile_dir, w, 'weights0')):
                print(pjoin(self.simpoint_profile_dir, w, 'weights0'), 'not found')
                flag = False
            if not osp.isfile(pjoin(self.simpoint_profile_dir, w, 'simpoints0')):
                flag = False
            return flag

        return False


    def run(self, t):
        workload, bbl_file, output_dir = t

        if not self.check_prereq(workload):
            print(f'{workload}`s prerequisite is not satisfied')
            return

        if not osp.isdir(output_dir):
            os.makedirs(output_dir)
        nemu = sh.Command('build/riscv64-nemu-interpreter')
        options = [
                bbl_file,
                '-C', self.task_name,
                '-w', workload,

                # common options
                '--sdcard-img', './rv-debian-spec-6G-fix-sphinx.img',
                '-b'
                ]
        options += self.extra_options
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

            return

        sh.rm(pjoin(output_dir, 'running'))
        sh.touch(pjoin(output_dir, 'completed'))
        return

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--spec-version', help='`06` or `17`',
            type=str, action='store', required=True)
    parser.add_argument('-w', '--workload', help='a specific workload',
            type=str, action='store', required=False)
    parser.add_argument('-t', '--task', help='task name',
            type=str, action='store', required=True,
            choices=[
                'simpoint_profile',
                'betapoint_profile',
                'take_simpoint_checkpoint',
                ])

    args = parser.parse_args()

    ver = args.spec_version
    bt = BatchTask(ver, args.task)

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

