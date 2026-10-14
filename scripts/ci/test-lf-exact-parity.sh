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

# Exact-cache leader-follower guard regression.
#
# Leader-follower profiling selects the exact block cache, under which the JIT
# declines to run (it charges the kept cache's shadow). This checks that a
# JIT-enabled binary used for LF therefore keeps the JIT off (0 translated
# blocks) and still produces the serial BBV with no rollbacks. It does NOT
# exercise the JIT under LF; that needs the kept-cache-leader decision.

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
workloads="${CI_WORKLOADS:-${repo_dir}/workloads}"
nemu_bin="${NEMU_BIN:-${repo_dir}/build/riscv64-nemu-interpreter}"
workload="${LF_EXACT_WORKLOAD:-${workloads}/linux/hello/fw_payload.bin}"
out_dir="${LF_EXACT_OUT_DIR:-${repo_dir}/nemu-lf-exact-logs}"

[ -f "$nemu_bin" ] || { echo "Required file not found: $nemu_bin" >&2; exit 1; }
[ -f "$workload" ] || { echo "Required file not found: $workload" >&2; exit 1; }

rm -rf "$out_dir"; mkdir -p "$out_dir/ser" "$out_dir/lf"

"$nemu_bin" -b --simpoint-profile --cpt-interval 1000000 --state-digest -D "$out_dir/ser" -w ci \
  -C t --fdt-rng-seed=1 --dont-skip-boot "$workload" > "$out_dir/ser.log" 2> "$out_dir/ser.console"
"$nemu_bin" -b --simpoint-profile --cpt-interval 1000000 --state-digest -D "$out_dir/lf" -w ci \
  -C lf --fdt-rng-seed=1 --dont-skip-boot --lf-profile --lf-slice=5000000 \
  --lf-followers=2 --lf-mem-track=off "$workload" > "$out_dir/lf.log" 2> "$out_dir/lf.console"

if grep -aq "CRITICAL" "$out_dir/lf.log"; then
  echo "FAIL: CRITICAL marker in the leader-follower log" >&2
  grep -an "CRITICAL" "$out_dir/lf.log" | head -n 10 >&2
  exit 1
fi

bs=$(find "$out_dir/ser" -name simpoint_bbv.gz | head -1)
bl=$(find "$out_dir/lf" -name simpoint_bbv.gz | head -1)
[ -n "$bs" ] && [ -n "$bl" ] || { echo "FAIL: missing BBV" >&2; exit 1; }

# Two empty profiles must not compare equal.
sb=$(zcat "$bs" | wc -c); lb=$(zcat "$bl" | wc -c)
if [ "$sb" -eq 0 ] || [ "$lb" -eq 0 ]; then
  echo "FAIL: empty decompressed BBV (serial $sb bytes, LF $lb bytes)" >&2
  exit 1
fi

# The LF run must have produced a positive number of slices and merged
# fragments; a bare label is not enough.
sl=$(sed -nE 's/.*leader-follower run ended at icount [0-9]+ after ([0-9]+) slices.*/\1/p' "$out_dir/lf.log" | tail -1)
fr=$(sed -nE 's/.*leader-follower merged ([0-9]+) fragments.*/\1/p' "$out_dir/lf.log" | tail -1)
if [ -z "$sl" ] || [ "$sl" -le 0 ] || [ -z "$fr" ] || [ "$fr" -le 0 ]; then
  echo "FAIL: the LF run produced no slices/fragments (slices='$sl', fragments='$fr')" >&2
  exit 1
fi

ms=$(zcat "$bs" | md5sum | cut -d' ' -f1)
ml=$(zcat "$bl" | md5sum | cut -d' ' -f1)
if [ "$ms" != "$ml" ]; then
  echo "FAIL: exact-cache LF BBV $ml differs from serial $ms" >&2
  exit 1
fi

# Parse the numeric rollback count out of the final summary line (the line
# continues with timing/statistics), so a substring such as "10 rollbacks"
# cannot pass.
if ! cmp -s "$out_dir/ser.console" "$out_dir/lf.console"; then
  echo "FAIL: the exact-cache LF guest console differs from serial" >&2
  exit 1
fi
ic_s=$(grep -aoE 'state digest: icount=[0-9]+' "$out_dir/ser.log" | tail -1 || true)
ic_l=$(grep -aoE 'state digest: icount=[0-9]+' "$out_dir/lf.log" | tail -1 || true)
if [ -z "$ic_s" ] || [ -z "$ic_l" ] || [ "$ic_s" != "$ic_l" ]; then
  echo "FAIL: instruction count differs from serial ($ic_s vs $ic_l)" >&2
  exit 1
fi

rollbacks=$(sed -nE 's/.*leader-follower summary: ([0-9]+) rollbacks.*/\1/p' "$out_dir/lf.log" | tail -1)
rollbacks_selfcheck=$(printf 'leader-follower summary: 10 rollbacks (1 us)\n' | sed -nE 's/.*leader-follower summary: ([0-9]+) rollbacks.*/\1/p')
if [ "$rollbacks_selfcheck" != "10" ]; then
  echo "FAIL: the rollback parser is broken (self-check got '$rollbacks_selfcheck')" >&2
  exit 1
fi
if [ "${rollbacks:-}" != "0" ]; then
  echo "FAIL: exact-cache LF rolled back (parsed '$rollbacks', expected 0)" >&2
  grep -a "rollbacks" "$out_dir/lf.log" >&2
  exit 1
fi

# The guard must actually hold: no translated block may run under LF.
if ! grep -qE "JIT: translated 0," "$out_dir/lf.log"; then
  echo "FAIL: JIT translated blocks under the exact-cache LF run" >&2
  grep -a "JIT: translated" "$out_dir/lf.log" >&2
  exit 1
fi

echo "PASS: exact-cache LF BBV equals serial ($ms), 0 rollbacks, JIT translated 0"
