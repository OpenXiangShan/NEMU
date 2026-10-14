#!/usr/bin/env bash
#***************************************************************************************
# Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
#
# NEMU is licensed under Mulan PSL v2.
#***************************************************************************************

# The --jit help line must name every option the parser accepts.

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
nemu_bin="${NEMU_BIN:-${repo_dir}/build/riscv64-nemu-interpreter}"
[ -f "$nemu_bin" ] || { echo "Required file not found: $nemu_bin" >&2; exit 1; }

help=$("$nemu_bin" --help 2>&1 | grep -a 'jit=K=V' | head -1)
[ -n "$help" ] || { echo "FAIL: no --jit help line" >&2; exit 1; }

keys=$(grep -oE 'strcmp\(k, "[a-z0-9-]+"' "$repo_dir/src/jit/jit.c" | sed 's/strcmp(k, "//; s/"//')
missing=0
for k in $keys; do
  case "$help" in
    *"$k"*) ;;
    *) echo "FAIL: --jit key '$k' is accepted but not documented" >&2; missing=1;;
  esac
done
[ "$missing" -eq 0 ] || exit 1
echo "PASS --jit help names every accepted option"
