import argparse
import subprocess
import os
import time
import signal

parser = argparse.ArgumentParser()
parser.add_argument("-n", "--num-thread", type=int, default=16)
args = parser.parse_args()

my_env = os.environ.copy()
my_env["NEMU_HOME"] = "/nfs/home/zhangchuanqi/lvna/for_xs/xs-env/nemu-sdcard-cpt/"

def run_take_simpoint_scripts(cluster,workloads,bbl_files):
    run_once_script = './onesimpoint.py'
    out_dir_path = './outputs'
    base_arguments = ["python3", run_once_script]
    proc_count, finish_count = 0, 0
    max_pending_proc = args.num_thread
    pending_proc, error_proc = [], []
    free_cores = list(range(max_pending_proc))

    # generate new full workloads
    a = []
    for w,bbl in zip(workloads,bbl_files):
        d = {}
        d['--cluster'] = f'{cluster}'
        d['--cpt-interval'] = '100000000'
        d['-w'] = f'{w}'
        d['-b'] = f'{bbl}'
        a.append(d)
    workloads = a

    try:
        while len(workloads) > 0 or len(pending_proc) > 0:
            has_pending_workload = len(workloads) > 0 and len(
                pending_proc) >= max_pending_proc
            has_pending_proc = len(pending_proc) > 0
            if has_pending_workload or has_pending_proc:
                finished_proc = list(
                    filter(lambda p: p[1].poll() is not None, pending_proc))
                for workload, proc, core in finished_proc:
                    print(f"{workload} has finished")
                    pending_proc.remove((workload, proc, core))
                    free_cores.append(core)
                    if proc.returncode != 0:
                        print(
                            f"[ERROR] {workload} exits with code {proc.returncode}")
                        error_proc.append(workload)
                        continue
                    finish_count += 1
                if len(finished_proc) == 0:
                    time.sleep(1)
            can_launch = max_pending_proc - len(pending_proc)
            for workload in workloads[:can_launch]:
                if len(pending_proc) < max_pending_proc:
                    allocate_core = free_cores[0]
                    addition_cmd = []
                    for k, v in workload.items():
                        if v is not None:
                            addition_cmd.append(f'{k}={v}')
                        else:
                            addition_cmd.append(f'{k}')
                    workload_name = workload['-w']
                    result_path = os.path.join(out_dir_path, f"{cluster}", f"{workload_name}")
                    if not os.path.exists(result_path):
                        os.makedirs(result_path, exist_ok=True)
                    stdout_file = os.path.join(result_path, "stdout.log")
                    stderr_file = os.path.join(result_path, "stderr.log")
                    with open(stdout_file, "w") as stdout, open(stderr_file, "w") as stderr:
                        run_cmd = base_arguments + addition_cmd
                        cmd_str = " ".join(run_cmd)
                        print(f"cmd {proc_count}: {cmd_str}")
                        proc = subprocess.Popen(
                            run_cmd, stdout=stdout, stderr=stderr, preexec_fn=os.setsid, env=my_env)
                        time.sleep(0.5)
                    pending_proc.append((workload, proc, allocate_core))
                    free_cores = free_cores[1:]
                    proc_count += 1
            workloads = workloads[can_launch:]
    except KeyboardInterrupt:
        print("Interrupted. Exiting all programs ...")
        print("Not finished:")
        for i, (workload, proc, _) in enumerate(pending_proc):
            os.killpg(os.getpgid(proc.pid), signal.SIGINT)
            print(f"  ({i + 1}) {workload}")
        print("Not started:")
        for i, workload in enumerate(workloads):
            print(f"  ({i + 1}) {workload}")
    if len(error_proc) > 0:
        print("Errors:")
        for i, workload in enumerate(error_proc):
            print(f"  ({i + 1}) {workload}")


spec_info = {
  "xapian": (),
  "img-dnn": (),
  "specjbb": (),
  "moses": (),
}

if __name__ == "__main__":
    # workloads = ['bt','cg','dc','ep','is','lu','mg','sp','ua']
    # workloads_name = [ f'{w}a' for w in workloads ]
    # bbl_files = [f'/nfs/home/zhangchuanqi/lvna/for_xs/xs-env/sw/riscv-pk/out_bins/npb_{bm}.bin' for bm in workloads_name]
    # run_take_simpoint_scripts("npb",workloads_name,bbl_files)

    # cluster = "gapbs"
    # workloads_name = ["bc","bfs","cc","cc_sv","pr","pr_spmv","sssp","tc"]
    # bbl_files = [f'/nfs/home/zhangchuanqi/lvna/for_xs/xs-env/sw/riscv-pk/out_bins/{cluster}/gap_{bm}.bin' for bm in workloads_name]
    # run_take_simpoint_scripts(cluster,workloads_name,bbl_files)

    # cluster = "branchopt"
    # workloads_name = ["dijkstra_opt","dijkstra_ori","905_ori","905_opt"]
    # bbl_files = [f'/nfs/home/zhangchuanqi/lvna/for_xs/xs-env/sw/riscv-pk/out_bins/{cluster}/branchopt_{bm}.bin' for bm in workloads_name]
    # run_take_simpoint_scripts(cluster,workloads_name,bbl_files)

    bin_cluster = "tailbench-withsd"
    cluster = "tailbench-withsd-50M"
    workloads_name = list(spec_info.keys())
    bbl_files = [f'/nfs/home/share/zhangchuanqi/cpt_bins/{bin_cluster}/{bm}.bin' for bm in workloads_name]
    run_take_simpoint_scripts(cluster,workloads_name,bbl_files)

    # pool = multiprocessing.Pool(processes=len(workloads))
    # for workload in workloads:
    #     pool.apply_async(run_take_simpoint_scripts, (workload, bbl_file, output_dir))
    # pool.close()
    # pool.join()