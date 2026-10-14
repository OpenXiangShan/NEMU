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

# Leader-follower SimPoint profiling must produce the serial profile: runs a
# workload serially and in leader-follower mode, with and without an injected
# leader fault (which must be rolled back), and compares the BBVs and the
# guest console.

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
workloads="${CI_WORKLOADS:-${repo_dir}/workloads}"
nemu_bin="${NEMU_BIN:-${repo_dir}/build/riscv64-nemu-interpreter}"
workload="${LF_WORKLOAD:-${workloads}/linux/hello/fw_payload.bin}"
out_dir="${LF_OUT_DIR:-${repo_dir}/nemu-lf-logs}"
interval="${LF_INTERVAL:-1000000}"
slice="${LF_SLICE:-5000000}"
inject_at="${LF_INJECT_AT:-30000000}"

for f in "$nemu_bin" "$workload"; do
  if [ ! -f "$f" ]; then
    echo "Required file not found: $f" >&2
    exit 1
  fi
done
rm -rf "$out_dir"
mkdir -p "$out_dir"

# Write tracking needs userfaultfd write-protection and PAGEMAP_SCAN (Linux
# 6.7 or later); without them, followers validate without the memory digest.
mem_track=uffd
if ! "$nemu_bin" -b --lf-selftest=memtrack "$workload" > "$out_dir/memtrack.log" 2>&1; then
  echo "::warning title=Leader-follower write tracking unavailable::validating without the memory digest"
  tail -n 20 "$out_dir/memtrack.log"
  mem_track=off
else
  # The tracker's own unit test, on synthetic mappings.
  make -s -C "$repo_dir/tools/lf-memtrack-test" NEMU_HOME="$repo_dir"
  "$repo_dir/tools/lf-memtrack-test/memtrack_test" > "$out_dir/memtrack-unit.log" 2>&1 || {
    echo "FAIL memtrack unit test" >&2
    tail -n 50 "$out_dir/memtrack-unit.log" >&2
    exit 1
  }
  make -s -C "$repo_dir/tools/lf-memtrack-test" clean
  echo "PASS memtrack unit test"
fi

run() {
  local name=$1
  shift
  local status=0
  "$nemu_bin" -b --fdt-rng-seed=1 --simpoint-profile --cpt-interval "$interval" \
    -D "$out_dir/$name" -w ci -C lf --dont-skip-boot "$@" "$workload" \
    > "$out_dir/$name.log" 2> "$out_dir/$name.console" || status=$?
  if [ "$status" -ne 0 ]; then
    echo "FAIL $name: exit status $status" >&2
    tail -n 50 "$out_dir/$name.log" >&2
    exit 1
  fi
  zcat "$(find "$out_dir/$name" -name simpoint_bbv.gz)" > "$out_dir/$name.bbv"
}

check_same() {
  local name=$1
  if ! cmp -s "$out_dir/serial.bbv" "$out_dir/$name.bbv"; then
    echo "FAIL $name: BBV differs from the serial run" >&2
    diff "$out_dir/serial.bbv" "$out_dir/$name.bbv" | head -n 20 >&2 || true
    exit 1
  fi
  if ! cmp -s "$out_dir/serial.console" "$out_dir/$name.console"; then
    echo "FAIL $name: guest console differs from the serial run" >&2
    exit 1
  fi
}

lf_args=(--lf-profile --lf-slice="$slice" --lf-followers=2 --lf-mem-track="$mem_track")

run serial
echo "serial: $(wc -l < "$out_dir/serial.bbv") intervals"
run lf "${lf_args[@]}"
check_same lf
echo "PASS leader-follower BBV identical to the serial run"

run lf-inject "${lf_args[@]}" --lf-inject=gpr@"$inject_at"
check_same lf-inject
if ! grep -q "leader-follower summary: 1 rollbacks" "$out_dir/lf-inject.log"; then
  echo "FAIL lf-inject: the injected leader fault was not rolled back once" >&2
  grep -a "leader-follower summary" "$out_dir/lf-inject.log" >&2 || true
  exit 1
fi
echo "PASS injected leader fault rolled back, BBV identical to the serial run"
