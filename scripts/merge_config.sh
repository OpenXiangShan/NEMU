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

# Merge NEMU configuration layers into one kconfig-format file.
#
# A layer is a plain .config-style file.  It may pull in another layer with a
# line
#
#     # fragment: <path>
#
# where <path> is relative to the directory of the file that contains the line.
# Layers are merged in the order given on the command line; for every CONFIG_*
# symbol the last definition wins, so a later layer overrides an earlier one.
#
# Usage:
#   scripts/merge_config.sh [-o OUTPUT] LAYER [LAYER...]
#   scripts/merge_config.sh -l LAYER [LAYER...]
#
# Without -o the merged configuration is written to stdout.  Non-configuration
# lines (comments, blank lines) are dropped from the output.  With -l the
# resolved layers are printed instead of the merged configuration.

set -euo pipefail

usage() {
  sed -n 's/^# \{0,1\}//p' <<'EOF'
Usage: scripts/merge_config.sh [-o OUTPUT] LAYER [LAYER...]

Merge NEMU configuration layers into a single kconfig-format file.

  -o OUTPUT   write the merged configuration to OUTPUT instead of stdout
  -l, --list  print the resolved layers (one per line) instead of the config
  -h          show this help
EOF
}

out=""
list=0
layers=()

while [ "$#" -gt 0 ]; do
  case "$1" in
    -o)
      [ "$#" -ge 2 ] || { echo "merge_config: -o needs an argument" >&2; exit 2; }
      out="$2"
      shift 2
      ;;
    -l|--list)
      list=1
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

declare -A seen=()
stack=()

resolve() {
  local file dir line inc i
  file="$(realpath "$1")" || {
    echo "merge_config: cannot resolve layer '$1'" >&2
    exit 1
  }
  [ -f "$file" ] || { echo "merge_config: no such layer: $1" >&2; exit 1; }

  for i in "${stack[@]}"; do
    if [ "$i" = "$file" ]; then
      echo "merge_config: include cycle through '$file'" >&2
      exit 1
    fi
  done

  # A layer that is pulled in twice is merged once: the layers form a DAG.
  if [ -n "${seen[$file]:-}" ]; then
    return 0
  fi
  seen[$file]=1

  stack+=("$file")
  dir="$(dirname "$file")"

  while IFS= read -r line || [ -n "$line" ]; do
    if [[ "$line" =~ ^#[[:space:]]*fragment:[[:space:]]*(.*)$ ]]; then
      inc="${BASH_REMATCH[1]}"
      inc="${inc%%[[:space:]]*}"   # drop a trailing comment
      [ -n "$inc" ] || { echo "merge_config: empty '# fragment:' in $file" >&2; exit 1; }
      resolve "$dir/$inc"
    elif [ "$list" != 1 ]; then
      printf '%s\n' "$line" >>"$tmp"
    fi
  done <"$file"

  unset "stack[$((${#stack[@]} - 1))]"

  # In --list mode print the file after the layers it includes, which is the
  # order in which a file with leading includes is effectively applied.
  if [ "$list" = 1 ]; then
    printf '%s\n' "$file" >>"$tmp"
  fi
}

: >"$tmp"
for layer in "${layers[@]}"; do
  resolve "$layer"
done

# Keep only configuration lines, and only the last definition of each symbol.
merge_awk='
/^CONFIG_[A-Za-z0-9_]+=/ {
  key = $0
  sub(/=.*/, "", key)
}
/^# CONFIG_[A-Za-z0-9_]+ is not set$/ {
  key = $0
  sub(/^# /, "", key)
  sub(/ is not set$/, "", key)
}
/^CONFIG_[A-Za-z0-9_]+=/ || /^# CONFIG_[A-Za-z0-9_]+ is not set$/ {
  if (key in slot) {
    value[slot[key]] = $0
  } else {
    slot[key] = ++count
    order[count] = key
    value[count] = $0
  }
  next
}
END {
  for (i = 1; i <= count; i++) {
    print value[i]
  }
}
'

if [ "$list" = 1 ]; then
  if [ -n "$out" ]; then
    cat "$tmp" >"$out"
  else
    cat "$tmp"
  fi
elif [ -n "$out" ]; then
  awk "$merge_awk" "$tmp" >"$out"
else
  awk "$merge_awk" "$tmp"
fi
