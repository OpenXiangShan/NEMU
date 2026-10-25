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

# 2. Layers are applied in the order given; the same file may be applied twice.
printf 'CONFIG_MSIZE=1\n' >"$tmp/base.config"
printf 'CONFIG_MSIZE=2\n' >"$tmp/overlay.config"
expect_out "later layer wins" 'CONFIG_MSIZE=2' "$tmp/base.config" "$tmp/overlay.config"
expect_out "a layer listed twice is re-applied" 'CONFIG_MSIZE=1' \
  "$tmp/base.config" "$tmp/overlay.config" "$tmp/base.config"

# 3. CRLF and trailing blanks on "# CONFIG_X is not set" stay effective.
printf 'CONFIG_X=y\r\n# CONFIG_X is not set\r\n' >"$tmp/crlf.config"
expect_out "CRLF unset line" '# CONFIG_X is not set' "$tmp/crlf.config"
printf 'CONFIG_X=y\n# CONFIG_X is not set   \n' >"$tmp/trailing.config"
expect_out "unset line with trailing blanks" '# CONFIG_X is not set' "$tmp/trailing.config"

# 4. -r reports redefinitions and redundant definitions.
printf 'CONFIG_A=y\nCONFIG_B=y\n' >"$tmp/one.config"
printf '# CONFIG_A is not set\nCONFIG_B=y\n' >"$tmp/two.config"
report="$("$merge" -r -o /dev/null "$tmp/one.config" "$tmp/two.config" 2>&1)"
case "$report" in
  *"override: CONFIG_A y -> n"*) ok "-r reports an override" ;;
  *) bad "-r reports an override"; printf '    got: %s\n' "$report" ;;
esac
case "$report" in
  *"redundant: CONFIG_B"*) ok "-r reports a redundant definition" ;;
  *) bad "-r reports a redundant definition"; printf '    got: %s\n' "$report" ;;
esac

case "$report" in
  *"$tmp/one.config -> $tmp/two.config"*) ok "-r shows both override sources" ;;
  *) bad "-r shows both override sources" ;;
esac

# 5. -o writes the merged configuration to a file.
if "$merge" -o "$tmp/out.config" "$tmp/base.config" && \
   [ "$(cat "$tmp/out.config")" = 'CONFIG_MSIZE=1' ]; then
  ok "-o writes the output file"
else
  bad "-o writes the output file"
fi

# 6. Broken inputs are rejected instead of silently dropped.
printf 'CONFIG_A\n' >"$tmp/malformed.config"
expect_fail "malformed configuration line" "$tmp/malformed.config"

printf '# CONFIG_A is not set please\n' >"$tmp/malformed-unset.config"
expect_fail "malformed unset line" "$tmp/malformed-unset.config"

printf ' CONFIG_MSIZE=0x40000000\n' >"$tmp/indented.config"
expect_fail "indented configuration line" "$tmp/indented.config"

expect_fail "missing layer" "$tmp/does-not-exist.config"

if [ "$fail" -ne 0 ]; then
  echo "merge_config_test: FAILED" >&2
  exit 1
fi
echo "merge_config_test: OK"
