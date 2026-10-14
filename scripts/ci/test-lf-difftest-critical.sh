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

# A run that exits 0 but prints the critical-error marker must not pass the
# leader-follower difftest runner.

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
runner="${repo_dir}/scripts/ci/run-lf-difftest.sh"

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

ref_so="${tmp}/ref.so"
: > "${ref_so}"
workload="${tmp}/fw_payload.bin"
: > "${workload}"

mock="${tmp}/mock-nemu"
cat > "${mock}" <<'MOCK'
#!/bin/sh
printf '\033[1;31mHIT CRITICAL ERROR\033[0m: injected for the runner test\n'
exit 0
MOCK
chmod +x "${mock}"

if NEMU_BIN="${mock}" REF_SO="${ref_so}" LF_WORKLOAD="${workload}" \
   LF_OUT_DIR="${tmp}/logs" bash "${runner}" > "${tmp}/out" 2>&1; then
  echo "FAIL: the leader-follower runner accepted a zero-exit CRITICAL mock" >&2
  cat "${tmp}/out" >&2
  exit 1
fi

if ! grep -q "CRITICAL" "${tmp}/out"; then
  echo "FAIL: the runner failed without naming the CRITICAL marker" >&2
  cat "${tmp}/out" >&2
  exit 1
fi

echo "PASS: a zero-exit CRITICAL leader-follower run is rejected"

# A zero-exit mock that prints the expected summary lines and writes a valid
# gzip of an empty stream must still be rejected: the profile carries no BBV.
mock2="${tmp}/mock-empty-bbv"
cat > "${mock2}" <<'MOCK'
#!/bin/sh
out=""
while [ $# -gt 0 ]; do
  case "$1" in
    -D) out="$2"; shift 2 ;;
    *) shift ;;
  esac
done
printf 'leader-follower summary: 1 slices, leader exec 1 us\n'
printf 'leader-follower merged 1 fragments\n'
mkdir -p "${out}/lf/ci"
: | gzip > "${out}/lf/ci/simpoint_bbv.gz"
exit 0
MOCK
chmod +x "${mock2}"

if NEMU_BIN="${mock2}" REF_SO="${ref_so}" LF_WORKLOAD="${workload}" \
   LF_OUT_DIR="${tmp}/logs2" bash "${runner}" > "${tmp}/out2" 2>&1; then
  echo "FAIL: the leader-follower runner accepted an empty BBV" >&2
  cat "${tmp}/out2" >&2
  exit 1
fi

if ! grep -q "BBV" "${tmp}/out2"; then
  echo "FAIL: the runner rejected the empty BBV without naming it" >&2
  cat "${tmp}/out2" >&2
  exit 1
fi

echo "PASS: an empty decompressed BBV is rejected"
