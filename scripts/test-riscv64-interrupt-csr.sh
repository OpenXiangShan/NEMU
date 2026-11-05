#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/.." && pwd)
ref_so=${1:-"$repo_dir/build/riscv64-nemu-interpreter-so"}
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT

# The library and headers must come from the same REF configuration.
"${CC:-cc}" -std=gnu11 -Wall -Werror -D__ISA__=riscv64 \
  -I"$repo_dir/include" -I"$repo_dir/lib-include" \
  -I"$repo_dir/src/isa/riscv64/include" \
  "$repo_dir/scripts/test-riscv64-interrupt-csr.c" \
  "$ref_so" -Wl,-rpath,"$(dirname "$ref_so")" -o "$test_dir/interrupt-csr"
"$test_dir/interrupt-csr"
