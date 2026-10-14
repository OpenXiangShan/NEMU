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

# A follower deliberately desynchronized from its reference must end the run
# with status 4 and must never be promoted as if the leader had diverged.

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
workloads="${CI_WORKLOADS:-${repo_dir}/workloads}"
nemu_bin="${NEMU_BIN:-${repo_dir}/build/riscv64-nemu-interpreter}"
ref_so="${REF_SO:-${SPIKE_SO:-}}"
workload="${LF_WORKLOAD:-${workloads}/linux/hello/fw_payload.bin}"
out_dir="${LF_OUT_DIR:-${repo_dir}/nemu-lf-desync-logs}"

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
  --cpt-interval 1000000 -D "$out_dir" -w ci -C lf --dont-skip-boot \
  --lf-profile --lf-slice=5000000 --lf-followers=2 --lf-mem-track=off \
  --lf-inject=desync@s0 \
  "$workload" > "$out_dir/run.log" 2>&1 || status=$?

if [ "$status" -ne 4 ]; then
  echo "FAIL: a difftest-desynced follower exited $status, expected 4" >&2
  tail -n 30 "$out_dir/run.log" >&2
  exit 1
fi

if ! grep -q "difftest mismatch aborted the follower" "$out_dir/run.log"; then
  echo "FAIL: the run ended without reporting the follower difftest mismatch" >&2
  tail -n 30 "$out_dir/run.log" >&2
  exit 1
fi

if grep -qE "becomes the leader|promot" "$out_dir/run.log"; then
  echo "FAIL: the desynchronized follower was promoted" >&2
  exit 1
fi

echo "PASS: a difftest-desynced follower ends the run with status 4 and is not promoted"
