#!/usr/bin/env bash
#***************************************************************************************
# Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#***************************************************************************************

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
workloads="${CI_WORKLOADS:-${repo_dir}/workloads}"
shared_workloads="${XIANGSHAN_CI_WORKLOADS:-/nfs/home/share/ci-workloads}"
spec_checkpoints="${SPEC_CHECKPOINTS_HOME:-/nfs/home/share/checkpoints_profiles}"
spec_slice_limit="${SPEC_SLICE_LIMIT:-1000}"
spec_slice_seed="${SPEC_SLICE_SEED:-${GITHUB_SHA:-default}}"
spec_max_instr="${SPEC_MAX_INSTR:-40000000}"
nemu_bin="${NEMU_BIN:-${repo_dir}/build/riscv64-nemu-interpreter}"
spike_so="${SPIKE_SO:-${repo_dir}/ready-to-run/spike-xiangshan-ref.so}"
log_dir="${NEMU_DIFF_LOG_DIR:-${repo_dir}/nemu-diff-spike-logs}"
if [ -n "${NEMU_DIFF_JOBS:-}" ]; then
  diff_jobs=$NEMU_DIFF_JOBS
  diff_jobs_configured=true
else
  diff_jobs=1
  diff_jobs_configured=false
fi
spec_slice_jobs="${SPEC_SLICE_JOBS:-}"
suite="all"
gcpt_restorer="${GCPT_RESTORE_BIN:-${shared_workloads}/fix-gcpt/gcpt.bin}"
case_extra_args=()
default_max_instr=""
case_index=0
case_total=""
manifest_file=""
manifest_lock_file=""
manifest_sorted_file=""
active_jobs=0
run_failures=0
compact_case_logs=false

usage() {
  cat <<'EOF'
Usage: scripts/ci/run-nemu-diff-spike-workloads.sh [options]

Run workload-builder images and checkpoint slices on NEMU with Spike DiffTest.

Options:
  --suite SUITE       Test suite to run.
  --max-instr NUM     Bounded instruction count passed to NEMU.
  --gcpt-restorer FILE
                     GCPT restorer used by spec-slices.
  --spec-root DIR     Root directory for SPEC checkpoint slices.
  --spec-limit NUM    Number of SPEC checkpoint slices to sample. Defaults to 1000.
  --spec-seed TEXT    Seed for deterministic pseudo-random sampling. Defaults to $GITHUB_SHA.
  --spec-max-instr NUM
                     Bounded instruction count for each SPEC checkpoint. Defaults to 40000000.
  --spec-jobs NUM     Number of SPEC checkpoint slices to run concurrently.
                     Defaults to SPEC_SLICE_JOBS, NEMU_DIFF_JOBS, or 64.
  --log-dir DIR       Directory for per-workload logs and manifest.
  --jobs NUM          Number of workloads to run concurrently. Defaults to 1.

Suites:
  basic       cputest, riscv-tests, rvv-test, misc-tests, linux hello, kvmtool
  advanced    long-running linux coremark-pro and rvv-bench smoke
  spec-slices  SPEC/checkpoint-style slices from the shared checkpoint corpus
  all         basic, advanced, and spec-slices

Environment:
  CI_WORKLOADS             Workload directory. Defaults to ./workloads.
  XIANGSHAN_CI_WORKLOADS   Shared XiangShan CI workload directory.
  SPEC_CHECKPOINTS_HOME    SPEC checkpoint root. Defaults to /nfs/home/share/checkpoints_profiles.
  SPEC_SLICE_LIMIT         Number of SPEC checkpoint slices to sample. Defaults to 1000.
  SPEC_SLICE_SEED          Seed for deterministic pseudo-random sampling. Defaults to $GITHUB_SHA.
  SPEC_MAX_INSTR           Bounded instruction count for each SPEC checkpoint. Defaults to 40000000.
  SPEC_SLICE_JOBS          Number of SPEC checkpoint slices to run concurrently.
                           Defaults to NEMU_DIFF_JOBS or 64.
  GCPT_RESTORE_BIN         GCPT restorer used by spec-slices.
  NEMU_BIN                 NEMU executable. Defaults to ./build/riscv64-nemu-interpreter.
  SPIKE_SO                 Spike difftest shared object. Defaults to ready-to-run/spike-xiangshan-ref.so.
  NEMU_DIFF_LOG_DIR        Directory for per-workload logs and manifest.
  NEMU_DIFF_JOBS           Number of workloads to run concurrently. Defaults to 1.
EOF
}

require_positive_integer() {
  local name=$1
  local value=$2
  case "$value" in
    ''|*[!0-9]*)
      echo "${name} must be a positive integer: ${value}" >&2
      exit 2
      ;;
  esac
  if [ "$value" -le 0 ]; then
    echo "${name} must be a positive integer: ${value}" >&2
    exit 2
  fi
}

require_file() {
  local path=$1
  if [ ! -f "$path" ]; then
    echo "Required file not found: $path" >&2
    exit 1
  fi
}

init_logs() {
  mkdir -p "$log_dir"
  manifest_file="${log_dir}/manifest.tsv"
  manifest_lock_file="${log_dir}/manifest.lock"
  manifest_sorted_file="${log_dir}/manifest.sorted.tsv"
  printf 'index\tstatus\tname\tworkload\tlog\tcommand\n' > "$manifest_file"
  echo "NEMU Spike DiffTest logs: $log_dir"
  echo "NEMU Spike DiffTest manifest: $manifest_file"
  echo "NEMU Spike DiffTest jobs: $diff_jobs"
}

append_manifest() {
  local index=$1
  local status=$2
  local name=$3
  local workload=$4
  local log_file=$5
  local command_line=$6

  if command -v flock >/dev/null 2>&1; then
    (
      flock 9
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$index" "$status" "$name" "$workload" "$log_file" "$command_line" >> "$manifest_file"
    ) 9>"$manifest_lock_file"
  else
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$index" "$status" "$name" "$workload" "$log_file" "$command_line" >> "$manifest_file"
  fi
}

sort_manifest() {
  {
    head -n 1 "$manifest_file"
    tail -n +2 "$manifest_file" | sort -n -k1,1
  } > "$manifest_sorted_file"
  echo "NEMU Spike DiffTest sorted manifest: $manifest_sorted_file"
}

run_case_with_index() {
  local index=$1
  local total=$2
  local name=$3
  shift
  shift
  shift
  local -a nemu_args=(-b --diff "$spike_so")
  local -a cmd
  local case_hash case_label command_line log_file safe_name status workload

  if [ -n "$default_max_instr" ]; then
    nemu_args+=(-I "$default_max_instr")
  fi
  cmd=("$nemu_bin" "${nemu_args[@]}" "${case_extra_args[@]}" "$@")
  workload=${1:-}
  case_hash=$(printf '%s' "$name" | sha256sum | cut -c1-12)
  safe_name=$(printf '%s' "$name" | tr '/ ' '__' | tr -c 'A-Za-z0-9._-' '_' | cut -c1-120)
  log_file="${log_dir}/$(printf '%04d' "$index")-${case_hash}-${safe_name}.log"
  command_line=$(printf '%q ' "${cmd[@]}")
  case_label="#${index}"
  if [ -n "$total" ]; then
    case_label="#${index}/${total}"
  fi

  if [ "$compact_case_logs" != true ]; then
    echo "RUN case ${case_label}: ${name}"
    echo "  workload: ${workload}"
    echo "  log: ${log_file}"
    echo "  command: ${command_line}"
  fi
  if "${cmd[@]}" > "$log_file" 2>&1; then
    status=0
  else
    status=$?
  fi
  if [ "$status" -eq 0 ]; then
    append_manifest "$index" "PASS" "$name" "$workload" "$log_file" "$command_line"
    if [ "$compact_case_logs" = true ]; then
      printf '%s - pass\n' "$name"
    else
      echo "PASS case ${case_label}: ${name}"
    fi
    return 0
  fi

  append_manifest "$index" "FAIL" "$name" "$workload" "$log_file" "$command_line"
  echo "FAIL case ${case_label}: ${name}" >&2
  echo "workload: ${workload}" >&2
  echo "log: ${log_file}" >&2
  echo "command: ${command_line}" >&2
  echo "::error title=NEMU Spike DiffTest failed::case ${case_label}: ${name}; workload: ${workload}; log: ${log_file}"
  echo "Last 200 lines from ${log_file}:"
  tail -n 200 "$log_file" >&2 || true
  echo "FAIL case ${case_label}: ${name}" >&2
  return "$status"
}

run_case() {
  local name=$1
  shift
  case_index=$((case_index + 1))
  run_case_with_index "$case_index" "$case_total" "$name" "$@"
}

wait_for_one_case() {
  local status
  if wait -n; then
    status=0
  else
    status=$?
  fi
  active_jobs=$((active_jobs - 1))
  if [ "$status" -ne 0 ]; then
    run_failures=1
  fi
}

wait_for_case_slot() {
  while [ "$active_jobs" -ge "$diff_jobs" ]; do
    wait_for_one_case
  done
}

wait_for_cases() {
  while [ "$active_jobs" -gt 0 ]; do
    wait_for_one_case
  done
  sort_manifest
  if [ "$run_failures" -ne 0 ]; then
    run_failures=0
    return 1
  fi
  return 0
}

start_case() {
  if [ "$diff_jobs" -le 1 ]; then
    local status
    if run_case "$@"; then
      status=0
    else
      status=$?
    fi
    if [ "$status" -ne 0 ]; then
      run_failures=1
      sort_manifest
      return "$status"
    fi
    return
  fi

  wait_for_case_slot
  if [ "$run_failures" -ne 0 ]; then
    wait_for_cases
    return 1
  fi
  local name=$1
  shift
  case_index=$((case_index + 1))
  run_case_with_index "$case_index" "$case_total" "$name" "$@" &
  active_jobs=$((active_jobs + 1))
}

run_dir_bins() {
  local dir=$1
  shift
  local -a skip_tests=("$@")
  local test_bin test_name skip

  if [ ! -d "$dir" ]; then
    echo "Test directory not found: $dir" >&2
    exit 1
  fi

  while IFS= read -r -d '' test_bin; do
    test_name=$(basename "$test_bin")
    skip=false
    for skip_name in "${skip_tests[@]}"; do
      if [ "$test_name" = "$skip_name" ]; then
        skip=true
        break
      fi
    done
    if [ "$skip" = true ]; then
      echo "${test_bin} skipped."
      continue
    fi
    start_case "$test_name" "$test_bin"
  done < <(find "$dir" -type f -name "*.bin" -print0 | sort -z)
}

run_basic() {
  local -a riscv_skip_tests=(
    rv64mi-p-ld-hd-misaligned.bin
    rv64mi-p-ld-misaligned.bin
    rv64mi-p-lh-hd-misaligned.bin
    rv64mi-p-lh-misaligned.bin
    rv64mi-p-lw-hd-misaligned.bin
    rv64mi-p-lw-misaligned.bin
    rv64mi-p-sd-hd-misaligned.bin
    rv64mi-p-sd-misaligned.bin
    rv64mi-p-sh-hd-misaligned.bin
    rv64mi-p-sh-misaligned.bin
    rv64mi-p-sw-hd-misaligned.bin
    rv64mi-p-sw-misaligned.bin
  )
  local -a rvv_skip_tests=(
    vsetvli-0.bin
    vsetivli-0.bin
  )
  local -a misc_tests=(
    am/misc-tests/bin/bitmanip.bin
    am/coremark/bin/coremark-riscv64-xs-rv64gc-o2.bin
    am/coremark/bin/coremark-riscv64-xs-rv64gc-o3.bin
    am/coremark/bin/coremark-riscv64-xs-rv64gcb-o3.bin
    am/misc-tests/bin/amtest-riscv64-xs.bin
    am/misc-tests/bin/aliastest-riscv64-xs.bin
    am/misc-tests/bin/softprefetchtest-riscv64-xs.bin
    am/misc-tests/bin/zacas-riscv64-xs.bin
  )
  local test_bin

  run_dir_bins "${workloads}/am/cputest/bin"
  run_dir_bins "${workloads}/am/riscv-tests/bin" "${riscv_skip_tests[@]}"
  run_dir_bins "${workloads}/am/riscv-vector-tests/bin" "${rvv_skip_tests[@]}"

  for test_bin in "${misc_tests[@]}"; do
    require_file "${workloads}/${test_bin}"
    start_case "$test_bin" "${workloads}/${test_bin}"
  done

  start_case "linux/hello/fw_payload.bin" "${workloads}/linux/hello/fw_payload.bin"
  start_case "linux/kvmtool/fw_payload.bin" "${workloads}/linux/kvmtool/fw_payload.bin"
  wait_for_cases
}

run_advanced() {
  start_case "linux/coremark-pro/fw_payload.bin" "${workloads}/linux/coremark-pro/fw_payload.bin" -I 400000000
  start_case "linux/rvv-bench/fw_payload.bin" "${workloads}/linux/rvv-bench/fw_payload.bin" -I 400000000
  wait_for_cases
}

run_spec_slices() {
  local test_bin total_tests
  local -a spec_tests
  local saved_compact_case_logs saved_diff_jobs

  require_positive_integer "SPEC_SLICE_LIMIT" "$spec_slice_limit"
  require_positive_integer "SPEC_MAX_INSTR" "$spec_max_instr"
  if [ -n "$spec_slice_jobs" ]; then
    require_positive_integer "SPEC_SLICE_JOBS" "$spec_slice_jobs"
  fi

  if [ ! -d "$spec_checkpoints" ]; then
    echo "SPEC checkpoint directory not found: $spec_checkpoints" >&2
    exit 1
  fi
  require_file "$gcpt_restorer"

  mapfile -t spec_tests < <(
    find "$spec_checkpoints" -type f \( -name "_*.gz" -o -name "_*.zstd" \) -print |
      python3 -c '
import hashlib
import os
import sys

seed = sys.argv[1]
limit = int(sys.argv[2])
root = os.path.abspath(sys.argv[3])

def spec_restorer(path):
    rel = os.path.relpath(path, root)
    parts = rel.split(os.sep)
    if len(parts) < 2:
        return None
    if not parts[0].startswith("spec06_"):
        return None
    corpus = os.path.join(root, parts[0])
    for candidate in (
        os.path.join(corpus, "gcpt", "gcpt.bin"),
        os.path.join(corpus, "build", "gcpt", "build", "gcpt.bin"),
    ):
        if os.path.isfile(candidate):
            return candidate
    return None

paths = []
for line in sys.stdin:
    path = line.rstrip("\n")
    if path and spec_restorer(path):
        paths.append(path)
paths.sort(key=lambda path: hashlib.sha256(f"{seed}\0{path}".encode()).hexdigest())
for path in paths[:limit]:
    print(path)
' "$spec_slice_seed" "$spec_slice_limit" "$spec_checkpoints"
  )
  total_tests=${#spec_tests[@]}
  if [ "$total_tests" -lt "$spec_slice_limit" ]; then
    echo "Need ${spec_slice_limit} SPEC checkpoint slices, found ${total_tests} under ${spec_checkpoints}" >&2
    exit 1
  fi

  echo "Selected ${total_tests} SPEC06 checkpoint slices from ${spec_checkpoints} with seed ${spec_slice_seed}."
  saved_diff_jobs=$diff_jobs
  saved_compact_case_logs=$compact_case_logs
  if [ -n "$spec_slice_jobs" ]; then
    diff_jobs=$spec_slice_jobs
  elif [ "$diff_jobs_configured" != true ]; then
    diff_jobs=64
  fi
  compact_case_logs=true
  echo "NEMU SPEC checkpoint jobs: $diff_jobs"
  case_total=$total_tests
  for test_bin in "${spec_tests[@]}"; do
    local rel_path corpus_dir spec_restorer
    rel_path=${test_bin#${spec_checkpoints}/}
    corpus_dir=${spec_checkpoints}/${rel_path%%/*}
    spec_restorer=""
    if [ -f "${corpus_dir}/gcpt/gcpt.bin" ]; then
      spec_restorer=${corpus_dir}/gcpt/gcpt.bin
    elif [ -f "${corpus_dir}/build/gcpt/build/gcpt.bin" ]; then
      spec_restorer=${corpus_dir}/build/gcpt/build/gcpt.bin
    fi
    if [ -z "$spec_restorer" ]; then
      echo "No GCPT restorer found for ${test_bin}" >&2
      exit 1
    fi
    start_case "spec-slices/${rel_path}" "$test_bin" -r "$spec_restorer" -I "$spec_max_instr"
  done
  wait_for_cases
  case_total=""
  diff_jobs=$saved_diff_jobs
  compact_case_logs=$saved_compact_case_logs
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --suite)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --suite" >&2
        exit 2
      fi
      suite=$2
      shift 2
      ;;
    --max-instr)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --max-instr" >&2
        exit 2
      fi
      default_max_instr=$2
      shift 2
      ;;
    --gcpt-restorer)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --gcpt-restorer" >&2
        exit 2
      fi
      gcpt_restorer=$2
      shift 2
      ;;
    --spec-root)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --spec-root" >&2
        exit 2
      fi
      spec_checkpoints=$2
      shift 2
      ;;
    --spec-limit)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --spec-limit" >&2
        exit 2
      fi
      spec_slice_limit=$2
      shift 2
      ;;
    --spec-seed)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --spec-seed" >&2
        exit 2
      fi
      spec_slice_seed=$2
      shift 2
      ;;
    --spec-max-instr)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --spec-max-instr" >&2
        exit 2
      fi
      spec_max_instr=$2
      shift 2
      ;;
    --spec-jobs)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --spec-jobs" >&2
        exit 2
      fi
      spec_slice_jobs=$2
      shift 2
      ;;
    --log-dir)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --log-dir" >&2
        exit 2
      fi
      log_dir=$2
      shift 2
      ;;
    --jobs)
      if [ "$#" -lt 2 ] || [ -z "$2" ]; then
        echo "Missing value for --jobs" >&2
        exit 2
      fi
      diff_jobs=$2
      diff_jobs_configured=true
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

case "$suite" in
  basic|advanced|spec-slices|all)
    ;;
  *)
    echo "Unknown suite: $suite" >&2
    usage >&2
    exit 2
    ;;
esac

require_file "$spike_so"
require_file "$nemu_bin"
require_positive_integer "NEMU_DIFF_JOBS" "$diff_jobs"
init_logs

case "$suite" in
  basic)
    run_basic
    ;;
  advanced)
    run_advanced
    ;;
  spec-slices)
    run_spec_slices
    ;;
  all)
    run_basic
    run_advanced
    run_spec_slices
    ;;
esac
