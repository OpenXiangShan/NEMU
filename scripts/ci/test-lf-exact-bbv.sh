#!/usr/bin/env bash
#***************************************************************************************
# Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
#
# NEMU is licensed under Mulan PSL v2.
#***************************************************************************************

set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
mock_dir=$(mktemp -d)
trap 'rm -rf "$mock_dir"' EXIT
payload="$mock_dir/payload"; printf 'BBVLINE\n' > "$payload"

mk_mock() {
  local name=$1 empty=$2 marker=$3 slices=$4 rollbacks=$5 translated=$6
  cat > "$mock_dir/$name" <<'MOCK'
#!/usr/bin/env bash
set -eu
out=""; lf=no
while [ $# -gt 0 ]; do
  case "$1" in
    -D) out=$2; shift 2;;
    --lf-profile) lf=yes; shift;;
    *) shift;;
  esac
done
[ -n "$out" ] || exit 0
mkdir -p "$out/ci"
if [ "$lf" = yes ] && [ "@EMPTY@" = yes ]; then : | gzip > "$out/ci/simpoint_bbv.gz"; else gzip -c "@PAYLOAD@" > "$out/ci/simpoint_bbv.gz"; fi
echo "state digest: icount=100"
if [ "$lf" = yes ]; then
  [ "@SLICES@" != 0 ] && echo "leader-follower run ended at icount 100 after @SLICES@ slices"
  echo "leader-follower merged 3 fragments"
  echo "leader-follower summary: @ROLLBACKS@ rollbacks (0 us)"
  echo "JIT: translated @TRANSLATED@, refused 0"
  [ "@MARKER@" = yes ] && echo "HIT CRITICAL ERROR"
fi
exit 0
MOCK
  sed -i "s#@EMPTY@#$empty#g; s#@SLICES@#$slices#g; s#@ROLLBACKS@#$rollbacks#g; s#@MARKER@#$marker#g; s#@PAYLOAD@#$payload#g; s#@TRANSLATED@#$translated#g" "$mock_dir/$name"
  chmod +x "$mock_dir/$name"
}
mk_mock clean      no  no  3 0 0
mk_mock marker     no  yes 3 0 0
mk_mock emptybbv   yes no  3 0 0
mk_mock zeroslice  no  no  0 0 0
mk_mock rollbacks  no  no  3 1 0
mk_mock translated no  no  3 0 5
run_with() {
  env NEMU_BIN="$1" LF_EXACT_WORKLOAD="$payload" LF_EXACT_OUT_DIR="$2" bash "$repo_dir/scripts/ci/test-lf-exact-parity.sh"
}
fail=0
if run_with "$mock_dir/clean" "$mock_dir/clean.d" > "$mock_dir/clean.out" 2>&1; then
  echo "PASS: clean mock accepted"
else
  echo "FAIL: clean mock rejected" >&2; cat "$mock_dir/clean.out" >&2; fail=1
fi
expect_fail() {
  local label=$1 diag=$2 out="$mock_dir/$1.out"
  if run_with "$mock_dir/$label" "$mock_dir/$label.d" > "$out" 2>&1; then
    echo "FAIL: $label was accepted" >&2; cat "$out" >&2; fail=1; return
  fi
  grep -qF "$diag" "$out" || { echo "FAIL: $label wrong diagnostic (wanted '$diag')" >&2; cat "$out" >&2; fail=1; return; }
  echo "PASS: $label rejected ($diag)"
}
expect_fail marker     "FAIL: CRITICAL marker in the leader-follower log"
expect_fail emptybbv   "FAIL: empty decompressed BBV"
expect_fail zeroslice  "FAIL: the LF run produced no slices/fragments"
expect_fail rollbacks  "FAIL: exact-cache LF rolled back"
expect_fail translated "FAIL: JIT translated blocks under the exact-cache LF run"
[ "$fail" -eq 0 ] || exit 1
