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

title=""
current=""
baseline=""

usage() {
  cat <<'EOF'
Usage: tools/perf-compare.sh --title TITLE --current FILE [--baseline FILE]

Read current and optional baseline TSV files produced by perf-benchmark.sh and
write a markdown comparison table to stdout.
EOF
}

require_value() {
  if [ "$#" -lt 2 ] || [ -z "$2" ]; then
    echo "Missing value for $1" >&2
    exit 2
  fi
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --title)
      require_value "$@"
      title="$2"
      shift 2
      ;;
    --current)
      require_value "$@"
      current="$2"
      shift 2
      ;;
    --baseline)
      require_value "$@"
      baseline="$2"
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

if [ -z "$title" ] || [ -z "$current" ]; then
  usage >&2
  exit 2
fi

python3 - "$title" "$current" "$baseline" <<'PY'
import csv
import sys
from pathlib import Path

title, current_path, baseline_path = sys.argv[1:4]


def parse_int(value):
    return int(value) if value not in ("", "N/A") else None


def parse_float(value):
    return float(value) if value not in ("", "N/A") else None


def read_metrics(path):
    if not path or not Path(path).is_file():
        return {}
    with open(path, newline="", encoding="utf-8") as fp:
        return {
            row["test"]: {
                "guest_instructions": parse_int(row["guest_instructions"]),
                "host_instructions": parse_int(row["host_instructions"]),
                "estimated_host_throughput": parse_float(row["estimated_host_throughput"]),
                "actual_nemu_throughput": parse_float(row["actual_nemu_throughput"]),
            }
            for row in csv.DictReader(fp, delimiter="\t")
        }


def sci(value):
    return "N/A" if value is None else f"{value:.3e}"


def pct_change(curr, base):
    if curr is None or base in (None, 0):
        return "N/A"
    return f"{(curr - base) * 100.0 / base:+.2f}%"


def instr_change(curr, base):
    if curr is None or base in (None, 0):
        return "N/A"
    return f"{(base - curr) * 100.0 / base:+.2f}%"


current_rows = read_metrics(current_path)
baseline_rows = read_metrics(baseline_path)

print(f"### {title}")
print()
print("| Test | Guest Instructions | Host Instructions | Estimated Host Throughput (instr/s) | Actual NEMU Throughput (instr/s) | Baseline Host Instructions | Baseline Actual NEMU Throughput (instr/s) | Change vs Baseline (Instructions) | Change vs Baseline (Throughput) |")
print("|------|--------------------|-------------------|-------------------------------------|----------------------------------|----------------------------|-------------------------------------------|-----------------------------------|---------------------------------|")
for test, row in current_rows.items():
    base = baseline_rows.get(test, {})
    host = row["host_instructions"]
    actual = row["actual_nemu_throughput"]
    base_host = base.get("host_instructions")
    base_actual = base.get("actual_nemu_throughput")
    print(
        f"| {test} | "
        f"{sci(row['guest_instructions'])} | "
        f"{sci(host)} | "
        f"{sci(row['estimated_host_throughput'])} | "
        f"{sci(actual)} | "
        f"{sci(base_host)} | "
        f"{sci(base_actual)} | "
        f"{instr_change(host, base_host)} | "
        f"{pct_change(actual, base_actual)} |"
    )
PY
