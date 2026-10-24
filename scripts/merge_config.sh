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
# A layer is a plain .config-style file.  It may pull in other layers with
# leading lines of the form
#
#     # fragment: <path>
#
# where <path> is relative to the directory of the file that contains the line.
# Includes are expanded in place and must precede the file's own CONFIG_ lines.
#
# Layers are merged in the order they are given; for every CONFIG_* symbol the
# last definition wins and it keeps the position of that last definition, so
# Kconfig choice semantics are preserved.
#
# Usage:
#   scripts/merge_config.sh [-o OUTPUT] LAYER [LAYER...]
#   scripts/merge_config.sh -l LAYER [LAYER...]
#
# Without -o the merged configuration is written to stdout.  With -l the
# resolved layers are printed in application order instead.

set -euo pipefail

usage() {
  sed -n 's/^# \{0,1\}//p' <<'EOF'
Usage: scripts/merge_config.sh [-o OUTPUT] LAYER [LAYER...]

Merge NEMU configuration layers into a single kconfig-format file.

  -o OUTPUT   write the merged configuration to OUTPUT instead of stdout
  -l, --list  print the resolved layers in application order
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

stack=()

# Append the CONFIG_ lines of one layer to $tmp, expanding leading includes.
resolve() {
  local file dir line inc sym i seen_config=0
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

  # Re-including a layer is allowed: the later include applies again, which is
  # what an ordered layer list promises.  Only true cycles are rejected.
  stack+=("$file")
  dir="$(dirname "$file")"

  while IFS= read -r line || [ -n "$line" ]; do
    line="${line%$'\r'}"

    if [[ "$line" =~ ^#[[:space:]]*fragment:[[:space:]]*(.*)$ ]]; then
      if [ "$seen_config" = 1 ]; then
        echo "merge_config: '$file': '# fragment:' lines must precede CONFIG_ lines" >&2
        exit 1
      fi
      inc="${BASH_REMATCH[1]}"
      inc="${inc%%[[:space:]]*}"   # drop a trailing comment
      [ -n "$inc" ] || { echo "merge_config: empty '# fragment:' in $file" >&2; exit 1; }
      resolve "$dir/$inc"
      continue
    fi

    # Canonicalize "# CONFIG_X is not set" (allow CRLF and trailing blanks).
    if [[ "$line" =~ ^#[[:space:]]*CONFIG_[A-Za-z0-9_]+([[:space:]]+is[[:space:]]+not[[:space:]]+set)[[:space:]]*$ ]]; then
      seen_config=1
      sym="${line#*CONFIG_}"
      sym="${sym%%[[:space:]]*}"
      if [ "$list" != 1 ]; then
        printf '# CONFIG_%s is not set\n' "$sym" >>"$tmp"
      fi
      continue
    fi

    if [[ "$line" =~ ^CONFIG_[A-Za-z0-9_]+= ]]; then
      seen_config=1
      if [ "$list" != 1 ]; then
        printf '%s\n' "$line" >>"$tmp"
      fi
      continue
    fi

    # Anything that looks like a configuration directive but did not parse is
    # an error: dropping it silently would change the effective configuration.
    if [[ "$line" == CONFIG_* ]] || [[ "$line" =~ ^#[[:space:]]*CONFIG_ ]]; then
      echo "merge_config: '$file': malformed configuration line: $line" >&2
      exit 1
    fi
  done <"$file"

  unset "stack[$((${#stack[@]} - 1))]"

  # In --list mode print the file after the layers it includes.  Includes are
  # leading, so this is the order in which the file is applied.
  if [ "$list" = 1 ]; then
    printf '%s\n' "$file" >>"$tmp"
  fi
}

: >"$tmp"
for layer in "${layers[@]}"; do
  resolve "$layer"
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
