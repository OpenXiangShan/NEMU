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
  local name=$1 empty=$2 marker=$3 slices=$4 frags=$5 digest=$6 lfdigest=${7:-same}
  cat > "$mock_dir/$name" <<'MOCK'
#!/usr/bin/env bash
set -eu
out=""; lf=no; inject=no
while [ $# -gt 0 ]; do
  case "$1" in
    -D) out=$2; shift 2;;
    --lf-profile) lf=yes; shift;;
    --lf-inject=*) inject=yes; shift;;
    *) shift;;
  esac
done
[ -n "$out" ] || exit 0
mkdir -p "$out/ci"
if [ "$lf" = yes ] && [ "@EMPTY@" = yes ]; then : | gzip > "$out/ci/simpoint_bbv.gz"; else gzip -c "@PAYLOAD@" > "$out/ci/simpoint_bbv.gz"; fi
if [ "$lf" = yes ]; then
  [ "@SLICES@" != 0 ] && echo "leader-follower run ended at icount 100 after @SLICES@ slices"
  [ "@FRAGS@" != 0 ] && echo "leader-follower merged @FRAGS@ fragments"
  [ "$inject" = yes ] && echo "leader-follower summary: 1 rollbacks (1 us)" || echo "leader-follower summary: 0 rollbacks (0 us)"
  echo "JIT: translated 0, refused 0"
  [ "@MARKER@" = yes ] && echo "HIT CRITICAL ERROR"
fi
dg="@DIGEST@"; [ "$lf" = yes ] && [ "@LFDIGEST@" != same ] && dg="@LFDIGEST@"
[ "$dg" != none ] && echo "state digest: icount=$dg"
exit 0
MOCK
  sed -i "s#@EMPTY@#$empty#g; s#@SLICES@#$slices#g; s#@FRAGS@#$frags#g; s#@MARKER@#$marker#g; s#@PAYLOAD@#$payload#g; s#@DIGEST@#$digest#g; s#@LFDIGEST@#$lfdigest#g" "$mock_dir/$name"
  chmod +x "$mock_dir/$name"
}

mk_mock clean    no  no  3 3 100 same
mk_mock marker   no  yes 3 3 100 same
mk_mock emptybbv yes no  3 3 100 same
mk_mock zeroslice no no  0 3 100 same
mk_mock zerofrag no  no  3 0 100 same
mk_mock nodigest no  no  3 3 none same
mk_mock mismatch no  no  3 3 100 999

run_with() {
  env NEMU_BIN="$1" LF_WORKLOAD="$payload" LF_OUT_DIR="$2" bash "$repo_dir/scripts/ci/run-lf-profiling.sh"
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
expect_fail marker    "FAIL lf: CRITICAL marker in the log"
expect_fail emptybbv  "FAIL lf: empty decompressed BBV"
expect_fail zeroslice "FAIL lf: no positive slice/fragment count"
expect_fail zerofrag  "FAIL lf: no positive slice/fragment count"
expect_fail nodigest  "FAIL lf: instruction count differs"
expect_fail mismatch  "FAIL lf: instruction count differs"
[ "$fail" -eq 0 ] || exit 1
