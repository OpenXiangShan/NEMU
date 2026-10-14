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

# Run leader-follower profiling with NEMU-NEMU difftest: the leader and the
# forked followers all run the exact engine, so every instruction is compared
# with the reference. The run must finish cleanly, without the critical-error
# marker, and must actually produce leader-follower slices, merged fragments
# and a BBV.

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
workloads="${CI_WORKLOADS:-${repo_dir}/workloads}"
nemu_bin="${NEMU_BIN:-${repo_dir}/build/riscv64-nemu-interpreter}"
ref_so="${REF_SO:-${SPIKE_SO:-}}"
workload="${LF_WORKLOAD:-${workloads}/linux/hello/fw_payload.bin}"
out_dir="${LF_OUT_DIR:-${repo_dir}/nemu-lf-difftest-logs}"
interval="${LF_INTERVAL:-1000000}"
slice="${LF_SLICE:-5000000}"

for f in "$nemu_bin" "$workload" "$ref_so"; do
  if [ ! -f "$f" ]; then
    echo "Required file not found: $f" >&2
    exit 1
  fi
done

rm -rf "$out_dir"
mkdir -p "$out_dir"

status=0
"$nemu_bin" -b --diff "$ref_so" --fdt-rng-seed=1 --simpoint-profile \
  --cpt-interval "$interval" -D "$out_dir" -w ci -C lf --dont-skip-boot \
  --lf-profile --lf-slice="$slice" --lf-followers=2 --lf-mem-track=off \
  "$workload" > "$out_dir/run.log" 2>&1 || status=$?

if [ "$status" -ne 0 ]; then
  echo "FAIL: leader-follower difftest run exited $status" >&2
  tail -n 50 "$out_dir/run.log" >&2
  exit 1
fi

# The critical-NMI path prints CRITICAL and then reports a successful halt, so
# a zero exit status alone is not enough to accept the run.
if grep -q "CRITICAL" "$out_dir/run.log"; then
  echo "FAIL: CRITICAL marker in the leader-follower log" >&2
  grep -n "CRITICAL" "$out_dir/run.log" | head -n 10 >&2
  exit 1
fi

if grep -q "is different after executing" "$out_dir/run.log"; then
  echo "FAIL: difftest mismatch in the leader-follower run" >&2
  grep -n "is different after executing" "$out_dir/run.log" | head -n 10 >&2
  exit 1
fi

if ! grep -Eq "leader-follower summary: [1-9][0-9]* slices" "$out_dir/run.log"; then
  echo "FAIL: the run produced no leader-follower slices" >&2
  tail -n 30 "$out_dir/run.log" >&2
  exit 1
fi

if ! grep -Eq "leader-follower merged [1-9][0-9]* fragments" "$out_dir/run.log"; then
  echo "FAIL: the run merged no leader-follower fragments" >&2
  tail -n 30 "$out_dir/run.log" >&2
  exit 1
fi

bbv=$(find "$out_dir" -name simpoint_bbv.gz | head -n 1)
bbv_bytes=0
if [ -n "$bbv" ] && [ -s "$bbv" ]; then
  bbv_bytes=$(zcat "$bbv" 2>/dev/null | wc -c)
fi
if [ "${bbv_bytes:-0}" -le 0 ]; then
  echo "FAIL: no merged, non-empty BBV was produced (decompressed $bbv_bytes bytes)" >&2
  exit 1
fi

echo "PASS leader-follower difftest against $(basename "$ref_so")"
