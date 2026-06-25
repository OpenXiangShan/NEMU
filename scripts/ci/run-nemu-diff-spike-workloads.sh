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
suite="all"

usage() {
  cat <<'EOF'
Usage: scripts/ci/run-nemu-diff-spike-workloads.sh [options]

Run workload-builder images on NEMU with Spike DiffTest.

Options:
  --suite SUITE       Test suite to run. Defaults to all.

Suites:
  basic       cputest, riscv-tests, rvv-test, misc-tests, linux hello, kvmtool
  advanced    long-running linux coremark-pro and rvv-bench smoke
  all         basic and advanced

Environment:
  CI_WORKLOADS   Workload directory. Defaults to ./workloads.
  NEMU_BIN       NEMU executable. Defaults to ./build/riscv64-nemu-interpreter.
  SPIKE_SO       Spike difftest shared object. Defaults to ready-to-run/spike-xiangshan-ref.so.
EOF
}

require_file() {
  local path=$1
  if [ ! -f "$path" ]; then
    echo "Required file not found: $path" >&2
    exit 1
  fi
}

run_case() {
  local name=$1
  shift
  echo "::group::${name}"
  echo "$nemu_bin" -b --diff "$spike_so" "$@"
  "$nemu_bin" -b --diff "$spike_so" "$@"
  echo "::endgroup::"
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
    run_case "$test_name" "$test_bin"
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
    run_case "$test_bin" "${workloads}/${test_bin}"
  done

  run_case "linux/hello/fw_payload.bin" "${workloads}/linux/hello/fw_payload.bin"
  run_case "linux/kvmtool/fw_payload.bin" "${workloads}/linux/kvmtool/fw_payload.bin"
}

run_advanced() {
  run_case "linux/coremark-pro/fw_payload.bin" "${workloads}/linux/coremark-pro/fw_payload.bin" -I 400000000
  run_case "linux/rvv-bench/fw_payload.bin" "${workloads}/linux/rvv-bench/fw_payload.bin" -I 400000000
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
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

require_file "$nemu_bin"
require_file "$spike_so"

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
  *)
    echo "Unknown suite: $suite" >&2
    usage >&2
    exit 2
    ;;
esac
