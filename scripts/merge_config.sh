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

# Merge configuration layers into one kconfig-format file.
#
# Layers are plain .config-style files given explicitly on the command line, in
# the order in which they must be applied (like the kernel's
# scripts/kconfig/merge_config.sh).  For every CONFIG_* symbol the last
# definition wins and it keeps the position of that last definition, so Kconfig
# choice semantics are preserved.
#
# Usage:
#   scripts/merge_config.sh [-r] [-o OUTPUT] LAYER [LAYER...]
#
#   -o OUTPUT   write the merged configuration to OUTPUT instead of stdout
#   -r          report redefined and redundant symbols on stderr
#   -h          show this help

set -euo pipefail

usage() {
  sed -n 's/^# \{0,1\}//p' <<'EOF'
Usage: scripts/merge_config.sh [-r] [-o OUTPUT] LAYER [LAYER...]

Merge configuration layers into a single kconfig-format file.

  -o OUTPUT   write the merged configuration to OUTPUT instead of stdout
  -r          report redefined and redundant symbols on stderr
  -h          show this help
EOF
}

out=""
report=0
layers=()

while [ "$#" -gt 0 ]; do
  case "$1" in
    -o)
      [ "$#" -ge 2 ] || { echo "merge_config: -o needs an argument" >&2; exit 2; }
      out="$2"
      shift 2
      ;;
    -r)
      report=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      layers+=("$@")
      break
      ;;
    -*)
      echo "merge_config: unknown option '$1'" >&2
      usage >&2
      exit 2
      ;;
    *)
      layers+=("$1")
      shift
      ;;
  esac
done

[ "${#layers[@]}" -gt 0 ] || { usage >&2; exit 2; }

tmp=$(mktemp "${TMPDIR:-/tmp}/nemu-merge-config.XXXXXX")
trap 'rm -f "$tmp"' EXIT

declare -A seen_val=() seen_src=()

# Append the CONFIG_ lines of one layer to $tmp.
read_layer() {
  local file="$1" line sym val
  [ -f "$file" ] || { echo "merge_config: no such layer: $file" >&2; exit 1; }

  while IFS= read -r line || [ -n "$line" ]; do
    line="${line%$'\r'}"

    # Canonicalize "# CONFIG_X is not set" (allow CRLF and trailing blanks).
    if [[ "$line" =~ ^#[[:space:]]*CONFIG_[A-Za-z0-9_]+([[:space:]]+is[[:space:]]+not[[:space:]]+set)[[:space:]]*$ ]]; then
      sym="${line#*CONFIG_}"
      sym="${sym%%[[:space:]]*}"
      val="n"
      line="# CONFIG_${sym} is not set"
    elif [[ "$line" =~ ^CONFIG_[A-Za-z0-9_]+= ]]; then
      sym="${line%%=*}"
      sym="${sym#CONFIG_}"
      val="${line#*=}"
    elif [[ "$line" == CONFIG_* ]] || [[ "$line" =~ ^#[[:space:]]*CONFIG_ ]]; then
      # Anything that looks like a configuration directive but did not parse is
      # an error: dropping it silently would change the effective configuration.
      echo "merge_config: '$file': malformed configuration line: $line" >&2
      exit 1
    else
      # Comments (including the target manifests' "# layer:" lines) and blanks.
      continue
    fi

    if [ -n "${seen_src[$sym]:-}" ]; then
      if [ "$val" = "${seen_val[$sym]}" ]; then
        [ "$report" = 1 ] && echo "redundant: CONFIG_$sym (${file#"$PWD"/})" >&2
      else
        [ "$report" = 1 ] && echo "override: CONFIG_$sym ${seen_val[$sym]} -> $val (${file#"$PWD"/})" >&2
      fi
    fi
    seen_val[$sym]="$val"
    seen_src[$sym]="$file"
    printf '%s\n' "$line" >>"$tmp"
  done <"$file"
}

: >"$tmp"
for layer in "${layers[@]}"; do
  read_layer "$layer"
done

# Keep only configuration lines; for each symbol keep its LAST definition at
# the position of that last definition (Kconfig choice semantics depend on the
# order of the surviving lines, not only on their values).
merge_awk='
/^CONFIG_[A-Za-z0-9_]+=/ {
  key = $0
  sub(/=.*/, "", key)
  text[NR] = $0
  keys[NR] = key
  last[key] = NR
  next
}
/^# CONFIG_[A-Za-z0-9_]+ is not set$/ {
  key = $0
  sub(/^# /, "", key)
  sub(/ is not set$/, "", key)
  text[NR] = $0
  keys[NR] = key
  last[key] = NR
  next
}
{ next }
END {
  for (i = 1; i <= NR; i++) {
    if (keys[i] != "" && last[keys[i]] == i) {
      print text[i]
    }
  }
}
'

if [ -n "$out" ]; then
  awk "$merge_awk" "$tmp" >"$out"
else
  awk "$merge_awk" "$tmp"
fi
