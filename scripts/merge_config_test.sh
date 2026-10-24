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

# Regression tests for scripts/merge_config.sh.
#
# Run it directly or through `make config-check` (which is part of CI).

set -u

merge="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/merge_config.sh"
[ -x "$merge" ] || { echo "merge_config_test: missing $merge" >&2; exit 1; }

tmp="$(mktemp -d "${TMPDIR:-/tmp}/nemu-merge-test.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT

fail=0
ok() { printf '  ok   %s\n' "$1"; }
bad() { printf '  FAIL %s\n' "$1"; fail=1; }

expect_out() {
  local desc="$1" want="$2" got
  shift 2
  if ! got="$("$merge" "$@")"; then
    bad "$desc (merge failed)"
    return
  fi
  if [ "$got" = "$want" ]; then
    ok "$desc"
  else
    bad "$desc"
    printf '    want: %q\n    got:  %q\n' "$want" "$got"
  fi
}

expect_fail() {
  local desc="$1"
  shift
  if "$merge" "$@" >/dev/null 2>&1; then
    bad "$desc (expected the merge to fail)"
  else
    ok "$desc"
  fi
}

# 1. A symbol keeps the position of its last definition, so Kconfig choice
#    precedence is the same as in the original sequence.
printf 'CONFIG_CC_O3=y\nCONFIG_CC_O0=y\nCONFIG_CC_O3=y\n' >"$tmp/choice.config"
expect_out "last definition keeps its position" \
  $'CONFIG_CC_O0=y\nCONFIG_CC_O3=y' "$tmp/choice.config"

# 2. CRLF and trailing blanks on "# CONFIG_X is not set" stay effective.
printf 'CONFIG_X=y\r\n# CONFIG_X is not set\r\n' >"$tmp/crlf.config"
expect_out "CRLF unset line" '# CONFIG_X is not set' "$tmp/crlf.config"
printf 'CONFIG_X=y\n# CONFIG_X is not set   \n' >"$tmp/trailing.config"
expect_out "unset line with trailing blanks" '# CONFIG_X is not set' "$tmp/trailing.config"

# 3. A layer that is included twice is applied twice (ordered list semantics).
printf 'CONFIG_MSIZE=1\n' >"$tmp/base.config"
printf '# fragment: base.config\nCONFIG_MSIZE=2\n' >"$tmp/overlay.config"
printf '# fragment: overlay.config\n# fragment: base.config\n' >"$tmp/again.config"
expect_out "re-included layer is re-applied" 'CONFIG_MSIZE=1' "$tmp/again.config"

# 4. --list prints the layers in application order.
printf 'CONFIG_A=y\n' >"$tmp/leaf.config"
printf '# fragment: leaf.config\nCONFIG_B=y\n' >"$tmp/mid.config"
printf '# fragment: mid.config\nCONFIG_C=y\n' >"$tmp/top.config"
want_list="$(realpath "$tmp/leaf.config") $(realpath "$tmp/mid.config") $(realpath "$tmp/top.config")"
got_list="$("$merge" --list "$tmp/top.config" | tr '\n' ' ' | sed 's/ $//')"
if [ "$got_list" = "$want_list" ]; then
  ok "--list reports application order"
else
  bad "--list reports application order"
  printf '    want: %s\n    got:  %s\n' "$want_list" "$got_list"
fi

# 5. Broken inputs are rejected instead of silently dropped.
printf '# fragment: b.config\n' >"$tmp/a.config"
printf '# fragment: a.config\n' >"$tmp/b.config"
expect_fail "include cycle" "$tmp/a.config"

printf 'CONFIG_A=y\n# fragment: leaf.config\n' >"$tmp/late-include.config"
expect_fail "include after CONFIG_ lines" "$tmp/late-include.config"

printf 'CONFIG_A\n' >"$tmp/malformed.config"
expect_fail "malformed configuration line" "$tmp/malformed.config"

printf '# CONFIG_A is not set please\n' >"$tmp/malformed-unset.config"
expect_fail "malformed unset line" "$tmp/malformed-unset.config"

printf '# fragment: missing.config\n' >"$tmp/missing.config"
expect_fail "missing included layer" "$tmp/missing.config"

if [ "$fail" -ne 0 ]; then
  echo "merge_config_test: FAILED" >&2
  exit 1
fi
echo "merge_config_test: OK"
