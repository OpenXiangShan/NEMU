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
#**************************************************************************************/

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: tools/ref-so-runner/gen-xs-ref-perf-defconfig.sh PATH_TO_OUTPUT_DEFCONFIG [PATH_TO_BASE_DEFCONFIG]

Generate an XS performance defconfig for standalone ref-so benchmarking.
The script starts from `riscv64-xs-ref_defconfig`, prints the config diff, and
writes the generated output defconfig.
EOF
}

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  usage >&2
  exit 2
fi

output="$1"
base="${2:-$(dirname "$output")/riscv64-xs-ref_defconfig}"

if [ ! -f "$base" ]; then
  echo "Base defconfig not found: $base" >&2
  exit 1
fi

if [ "$(realpath -m "$output")" = "$(realpath -m "$base")" ]; then
  echo "Output defconfig must be different from the base defconfig" >&2
  exit 1
fi

tmp=$(mktemp)
cleanup() {
  rm -f "$tmp"
}
trap cleanup EXIT

cp "$base" "$tmp"

python3 - "$tmp" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()

updates = [
    (
        "CONFIG_RV_AIA=y\nCONFIG_RV_IMSIC=y\nCONFIG_GEILEN=5",
        "# CONFIG_RV_AIA is not set",
    ),
    (
        "# CONFIG_CLINT_LOCAL_TIMER_INTERRUPT is not set",
        "CONFIG_CLINT_LOCAL_TIMER_INTERRUPT=y\n"
        "CONFIG_CYCLES_PER_MTIME_TICK=128\n"
        "CONFIG_WFI_TIMEOUT_TICKS=8192",
    ),
    (
        "# CONFIG_HAS_UART16550 is not set",
        "CONFIG_HAS_UART16550=y\n"
        "CONFIG_UART16550_PORT=0x3f8\n"
        "CONFIG_UART16550_MMIO=0x310b0000\n"
        "# CONFIG_UART16550_REG_SHIFT_0 is not set\n"
        "CONFIG_UART16550_REG_SHIFT_2=y\n"
        "# CONFIG_UART16550_INPUT_FIFO is not set",
    ),
    (
        "# CONFIG_HAS_UARTLITE is not set",
        "CONFIG_HAS_UARTLITE=y\n"
        "CONFIG_UARTLITE_PORT=0x3f8\n"
        "CONFIG_UARTLITE_MMIO=0x40600000\n"
        "# CONFIG_UARTLITE_INPUT_FIFO is not set\n"
        "CONFIG_UARTLITE_ASSERT_FOUR=y",
    ),
    (
        "# CONFIG_HAS_PLIC is not set",
        "CONFIG_HAS_PLIC=y\n"
        "CONFIG_PLIC_ADDRESS=0x3c000000",
    ),
]

for old, new in updates:
    if new in text:
        continue
    if old not in text:
        raise SystemExit(f"Could not find expected config marker: {old}")
    text = text.replace(old, new, 1)

path.write_text(text)
PY

diff -u "$base" "$tmp" || true

if cmp -s "$tmp" "$output" 2>/dev/null; then
  echo "Generated defconfig already up to date: $output"
  exit 0
fi

cp "$tmp" "$output"
echo "Generated $output from $base"
