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

# Compose one configuration target from its composition manifest.
#
# A manifest (configs/<name>_defconfig) lists its layers with
#
#     # layer: <path>
#
# lines, where <path> is relative to configs/.  This script validates those
# lines, resolves them against configs/, appends the manifest itself (for its
# target-specific overrides) and runs scripts/merge_config.sh on the result.
# Files without layer directives are ordinary defconfigs.
# Both `make <name>_defconfig` and scripts/check_configs.sh use this parser, so
# they cannot disagree about which layers a target has.
#
# Usage:
#   scripts/merge_target.sh [--list] [-r] -o OUTPUT MANIFEST
#
#   --list      print the resolved layers (absolute paths, application order)
#   -r          pass -r (report overridden/redundant symbols) to merge_config
#   -o OUTPUT   merged configuration output (required unless --list)
#   -h          show this help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NEMU_HOME="${NEMU_HOME:-$(cd "$SCRIPT_DIR/.." && pwd)}"
export NEMU_HOME

CONFIGS="$NEMU_HOME/configs"
MERGE="$SCRIPT_DIR/merge_config.sh"

usage() {
  sed -n 's/^# \{0,1\}//p' <<'EOF'
Usage: scripts/merge_target.sh [--list] [-r] -o OUTPUT MANIFEST

Compose one configuration target from its composition manifest.

  --list      print the resolved layers (absolute paths, application order)
  -r          report overridden and redundant symbols
  -o OUTPUT   write the merged configuration to OUTPUT
  -h          show this help
EOF
}

out=""
list=0
report=0
manifest=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    -o)
      [ "$#" -ge 2 ] || { echo "merge_target: -o needs an argument" >&2; exit 2; }
      out="$2"
      shift 2
      ;;
    --list)
      list=1
      shift
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
      manifest="${1:-}"
      shift || true
      ;;
    -*)
      echo "merge_target: unknown option '$1'" >&2
      usage >&2
      exit 2
      ;;
    *)
      [ -z "$manifest" ] || { echo "merge_target: more than one manifest given" >&2; exit 2; }
      manifest="$1"
      shift
      ;;
  esac
done

[ -n "$manifest" ] || { usage >&2; exit 2; }
[ -f "$manifest" ] || { echo "merge_target: no such manifest: $manifest" >&2; exit 1; }

layers=()
lineno=0

while IFS= read -r line || [ -n "$line" ]; do
  lineno=$((lineno + 1))
  line="${line%$'\r'}"   # tolerate CRLF manifests

  # An indented layer directive would be silently ignored; reject it.
  if [[ "$line" =~ ^[[:space:]]+#[[:space:]]*layer([^A-Za-z0-9_]|$) ]]; then
    echo "merge_target: $manifest:$lineno: layer directive must not be indented: $line" >&2
    exit 1
  fi

  # Any line that starts a "layer" word is a directive and must parse; other
  # comments (including "# target: ...") are ignored.
  if [[ "$line" =~ ^#[[:space:]]*layer([^A-Za-z0-9_]|$) ]]; then
    if [[ "$line" =~ ^#[[:space:]]*layer:[[:space:]]*([^[:space:]]+)[[:space:]]*$ ]]; then
      rel="${BASH_REMATCH[1]}"
      case "$rel" in
        /*)
          echo "merge_target: $manifest:$lineno: layer path must be relative to configs/: $rel" >&2
          exit 1
          ;;
        *..*)
          echo "merge_target: $manifest:$lineno: layer path must not contain '..': $rel" >&2
          exit 1
          ;;
        *[!A-Za-z0-9._/-]*)
          echo "merge_target: $manifest:$lineno: unsupported character in layer path: $rel" >&2
          exit 1
          ;;
      esac
      abs="$CONFIGS/$rel"
      if [ ! -f "$abs" ]; then
        echo "merge_target: $manifest:$lineno: layer not found: configs/$rel" >&2
        exit 1
      fi
      layers+=("$(realpath "$abs")")
    else
      echo "merge_target: $manifest:$lineno: malformed layer directive (expected '# layer: <path>'): $line" >&2
      exit 1
    fi
  fi
done <"$manifest"

if [ "$list" = 1 ]; then
  if [ "${#layers[@]}" -gt 0 ]; then
    printf '%s\n' "${layers[@]}"
  fi
  exit 0
fi

[ -n "$out" ] || { usage >&2; exit 2; }

merge_args=()
[ "$report" = 1 ] && merge_args+=(-r)

# Append the manifest itself: it carries the target-specific overrides.
"$MERGE" "${merge_args[@]}" -o "$out" "${layers[@]}" "$manifest"
