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

# Regression tests for the composition-manifest parser (scripts/merge_target.sh).
#
# Run it directly or through `make config-check` (which is part of CI).

set -u

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
merge_target="$here/merge_target.sh"
[ -x "$merge_target" ] || { echo "merge_target_test: missing $merge_target" >&2; exit 1; }

# The parser resolves layers against $NEMU_HOME/configs, so build a throw-away
# tree with the same layout.
NEMU_HOME="$(mktemp -d "${TMPDIR:-/tmp}/nemu-merge-target.XXXXXX")"
export NEMU_HOME
mkdir -p "$NEMU_HOME/configs/riscv64" "$NEMU_HOME/configs/fragments"
printf 'CONFIG_A=y\n' >"$NEMU_HOME/configs/riscv64/prof.config"
printf 'CONFIG_B=y\n' >"$NEMU_HOME/configs/fragments/frag.config"
trap 'rm -rf "$NEMU_HOME"' EXIT

fail=0
ok() { printf '  ok   %s\n' "$1"; }
bad() { printf '  FAIL %s\n' "$1"; fail=1; }

expect_list() {
  local desc="$1" want="$2" file="$3" got
  if ! got="$("$merge_target" --list "$file")"; then
    bad "$desc (parser failed)"
    return
  fi
  if [ "$got" = "$want" ]; then
    ok "$desc"
  else
    bad "$desc"
    printf '    want: %s\n    got:  %s\n' "$want" "$got"
  fi
}

expect_fail() {
  local desc="$1" file="$2"
  if "$merge_target" --list "$file" >/dev/null 2>&1; then
    bad "$desc (expected the parser to fail)"
  else
    ok "$desc"
  fi
}

# Plain manifest.
printf '# NEMU configuration target: demo\n# layer: riscv64/prof.config\n#\nCONFIG_C=y\n' >"$NEMU_HOME/m1"
expect_list "plain manifest" \
  "$(realpath "$NEMU_HOME/configs/riscv64/prof.config")" "$NEMU_HOME/m1"

# Tab after the colon, trailing blanks and CRLF are accepted and normalized.
printf '# layer:\triscv64/prof.config\r\n# layer: fragments/frag.config   \r\n' >"$NEMU_HOME/m2"
expect_list "tab / trailing blanks / CRLF" \
  "$(realpath "$NEMU_HOME/configs/riscv64/prof.config")
$(realpath "$NEMU_HOME/configs/fragments/frag.config")" "$NEMU_HOME/m2"

# Malformed directives are rejected instead of dropping the layer.
printf '# layer : riscv64/prof.config\n' >"$NEMU_HOME/bad1"
expect_fail "space before the colon" "$NEMU_HOME/bad1"

printf '# layer:\n' >"$NEMU_HOME/bad2"
expect_fail "empty layer path" "$NEMU_HOME/bad2"

printf '# layer: riscv64/prof.config # trailing comment\n' >"$NEMU_HOME/bad3"
expect_fail "path followed by a comment" "$NEMU_HOME/bad3"

printf '# layer: riscv64/missing.config\n' >"$NEMU_HOME/bad4"
expect_fail "missing layer" "$NEMU_HOME/bad4"

printf '# layer: ../outside.config\n' >"$NEMU_HOME/bad5"
expect_fail "path with .." "$NEMU_HOME/bad5"

printf '# layer: riscv64/prof config\n' >"$NEMU_HOME/bad6"
expect_fail "path with a space" "$NEMU_HOME/bad6"

printf '  # layer: riscv64/prof.config\n' >"$NEMU_HOME/bad8"
expect_fail "indented layer directive" "$NEMU_HOME/bad8"

printf '# no layers here\n' >"$NEMU_HOME/bad7"
expect_fail "manifest without layers" "$NEMU_HOME/bad7"

# The manifest's own overrides are merged last.
printf '# layer: riscv64/prof.config\nCONFIG_A=n\n' >"$NEMU_HOME/m3"
got="$("$merge_target" -o /dev/stdout "$NEMU_HOME/m3")"
if [ "$got" = "CONFIG_A=n" ]; then
  ok "manifest overrides are merged last"
else
  bad "manifest overrides are merged last"
  printf '    got: %q\n' "$got"
fi

if [ "$fail" -ne 0 ]; then
  echo "merge_target_test: FAILED" >&2
  exit 1
fi
echo "merge_target_test: OK"
