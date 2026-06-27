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
nemu_bin="${NEMU_BIN:-${repo_dir}/build/riscv64-nemu-interpreter}"
spike_so="${SPIKE_SO:-${repo_dir}/ready-to-run/spike-xiangshan-ref.so}"
log_dir="${NEMU_DIFF_LOG_DIR:-${repo_dir}/nemu-diff-spike-logs}"
if [ -n "${NEMU_DIFF_JOBS:-}" ]; then
  diff_jobs=$NEMU_DIFF_JOBS
else
  diff_jobs=1
fi
suite="all"
case_extra_args=()
default_max_instr=""
case_index=0
case_total=""
manifest_file=""
manifest_lock_file=""
manifest_sorted_file=""
active_jobs=0
run_failures=0

usage() {
  cat <<'EOF'
Usage: scripts/ci/run-nemu-diff-spike-workloads.sh [options]

Run workload-builder images on NEMU with Spike DiffTest.

Options:
  --suite SUITE       Test suite to run.
  --max-instr NUM     Bounded instruction count passed to NEMU.
  --log-dir DIR       Directory for per-workload logs and manifest.
  --jobs NUM          Number of workloads to run concurrently. Defaults to 1.

Suites:
  basic       cputest, riscv-tests, rvv-test, misc-tests, linux hello, kvmtool
  advanced    long-running linux coremark-pro and rvv-bench smoke
  all         basic and advanced

Environment:
  CI_WORKLOADS             Workload directory. Defaults to ./workloads.
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

  echo "RUN case ${case_label}: ${name}"
  echo "  workload: ${workload}"
  echo "  log: ${log_file}"
  echo "  command: ${command_line}"
  if "${cmd[@]}" > "$log_file" 2>&1; then
    status=0
  else
    status=$?
  fi
  if [ "$status" -eq 0 ]; then
    append_manifest "$index" "PASS" "$name" "$workload" "$log_file" "$command_line"
    echo "PASS case ${case_label}: ${name}"
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
  basic|advanced|all)
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
  all)
    run_basic
    run_advanced
    ;;
esac
