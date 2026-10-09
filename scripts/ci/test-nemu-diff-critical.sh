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

# A NEMU build that hits the critical-error path prints CRITICAL and then
# reports a successful halt, so a zero exit status alone would accept it.
# This checks that the workload runner rejects such a log. The runner is
# driven with a mock binary that always prints the marker and exits 0.

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
runner="${repo_dir}/scripts/ci/run-nemu-diff-spike-workloads.sh"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

workloads="${work}/workloads"
mkdir -p "${workloads}/am/cputest/bin" \
         "${workloads}/am/riscv-tests/bin" \
         "${workloads}/am/riscv-vector-tests/bin" \
         "${workloads}/am/misc-tests/bin" \
         "${workloads}/am/coremark/bin" \
         "${workloads}/linux/hello" \
         "${workloads}/linux/kvmtool"
: > "${workloads}/am/cputest/bin/mock.bin"
for name in bitmanip amtest-riscv64-xs aliastest-riscv64-xs \
            softprefetchtest-riscv64-xs zacas-riscv64-xs; do
  : > "${workloads}/am/misc-tests/bin/${name}.bin"
done
for name in coremark-riscv64-xs-rv64gc-o2 coremark-riscv64-xs-rv64gc-o3 \
            coremark-riscv64-xs-rv64gcb-o3; do
  : > "${workloads}/am/coremark/bin/${name}.bin"
done
: > "${workloads}/linux/hello/fw_payload.bin"
: > "${workloads}/linux/kvmtool/fw_payload.bin"

ref_so="${work}/ref.so"
: > "${ref_so}"

mock="${work}/mock-nemu"
cat > "${mock}" <<'MOCK'
#!/bin/sh
printf '\033[1;31mHIT CRITICAL ERROR\033[0m: injected for the runner test\n'
exit 0
MOCK
chmod +x "${mock}"

log_dir="${work}/logs"
if CI_WORKLOADS="${workloads}" NEMU_BIN="${mock}" REF_SO="${ref_so}" \
   NEMU_DIFF_JOBS=4 NEMU_DIFF_LOG_DIR="${log_dir}" \
   bash "${runner}" --suite basic > "${work}/out" 2>&1; then
  echo "FAIL: the runner accepted a zero-exit CRITICAL log" >&2
  tail -n 20 "${work}/out" >&2
  exit 1
fi

if ! grep -q "CRITICAL" "${work}/out"; then
  echo "FAIL: the runner failed without naming the CRITICAL marker" >&2
  tail -n 20 "${work}/out" >&2
  exit 1
fi

cases=$(awk -F'\t' 'NR > 1 { n++ } END { print n + 0 }' "${log_dir}/manifest.tsv")
passed=$(awk -F'\t' 'NR > 1 && $2 != "FAIL" { n++ } END { print n + 0 }' "${log_dir}/manifest.tsv")
if [ "${cases}" -le 0 ] || [ "${passed}" -ne 0 ]; then
  echo "FAIL: ${passed} of ${cases} cases were not marked FAIL" >&2
  cat "${log_dir}/manifest.tsv" >&2
  exit 1
fi

echo "PASS: a zero-exit CRITICAL log is rejected (${cases} cases)"