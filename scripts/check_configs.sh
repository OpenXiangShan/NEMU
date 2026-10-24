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

# Check the layered configuration tree in configs/:
#   1. every configs/*_defconfig target composes into a valid kconfig input,
#   2. composition is deterministic,
#   3. every CONFIG_ symbol mentioned by a layer exists in some Kconfig file,
#   4. every layer is used by at least one target (warning only).
#
# Run it directly or through `make config-check`.  CI runs it on every PR.

set -euo pipefail

NEMU_HOME="${NEMU_HOME:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
export NEMU_HOME

CONFIGS="$NEMU_HOME/configs"
KCONFIG="$NEMU_HOME/Kconfig"
MERGE="$NEMU_HOME/scripts/merge_config.sh"
CONF="$NEMU_HOME/tools/kconfig/build/conf"

color_red=$'\033[1;31m'
color_green=$'\033[1;32m'
color_bold=$'\033[1m'
color_end=$'\033[0m'

fail=0
problem() { printf '%s%s%s\n' "$color_red" "$*" "$color_end" >&2; fail=1; }

[ -f "$KCONFIG" ] || { echo "check_configs: $NEMU_HOME is not a NEMU tree" >&2; exit 1; }
[ -f "$MERGE" ] || { echo "check_configs: missing $MERGE" >&2; exit 1; }

tmp="$(mktemp -d "${TMPDIR:-/tmp}/nemu-check-configs.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT

printf '%sChecking configuration targets in %s%s\n' "$color_bold" "$CONFIGS" "$color_end"

make -s -C "$NEMU_HOME/tools/kconfig" NAME=conf
make -s -C "$NEMU_HOME/tools/fixdep"

# ---------------------------------------------------------------------------
# 0. The merger itself has regression tests.
# ---------------------------------------------------------------------------
if [ -x "$NEMU_HOME/scripts/merge_config_test.sh" ]; then
  if ! bash "$NEMU_HOME/scripts/merge_config_test.sh"; then
    echo "check_configs: the configuration merger failed its own tests" >&2
    exit 1
  fi
fi

# ---------------------------------------------------------------------------
# 1+2. Compose every target.
# ---------------------------------------------------------------------------
targets=("$CONFIGS"/*_defconfig)
[ "${#targets[@]}" -gt 0 ] || { echo "check_configs: no configs/*_defconfig found" >&2; exit 1; }

override_slots=0
redundant_slots=0

for target in "${targets[@]}"; do
  name="$(basename "$target")"
  merged="$tmp/$name.config"

  # The target manifest lists its layers as "# layer: <path>" lines relative
  # to configs/; the manifest itself is merged last for its own overrides.
  mapfile -t layer_rel < <(sed -n 's|^# layer: ||p' "$target")
  layer_abs=()
  for layer in "${layer_rel[@]}"; do
    layer_abs+=("$CONFIGS/$layer")
  done

  merge_diag="$tmp/$name.merge"
  if ! "$MERGE" -r -o "$merged" "${layer_abs[@]}" "$target" 2>"$merge_diag"; then
    problem "FAIL $name: cannot merge the configuration layers"
    sed 's/^/         /' "$merge_diag" >&2
    continue
  fi
  overrides=$(grep -c '^override:' "$merge_diag" || true)
  redundant=$(grep -c '^redundant:' "$merge_diag" || true)
  override_slots=$((override_slots + overrides))
  redundant_slots=$((redundant_slots + redundant))
  if [ "$redundant" -gt 0 ]; then
    printf '  note %s: %d redundant definition(s)\n' "$name" "$redundant"
  fi
  diag="$tmp/$name.diag"
  if ! KCONFIG_CONFIG="$tmp/$name.out" "$CONF" -s --defconfig="$merged" "$KCONFIG" 2>"$diag"; then
    problem "FAIL $name: Kconfig rejected the merged configuration"
    sed 's/^/         /' "$diag" >&2
    continue
  fi
  if [ -s "$diag" ]; then
    problem "FAIL $name: Kconfig reported diagnostics"
    sed 's/^/         /' "$diag" >&2
    continue
  fi
  case "$name" in
    riscv64-*) want_isa=CONFIG_ISA_riscv64=y ;;
    riscv32-*) want_isa=CONFIG_ISA_riscv32=y ;;
    *)         want_isa="" ;;
  esac
  if [ -n "$want_isa" ] && ! grep -qx "$want_isa" "$tmp/$name.out"; then
    problem "FAIL $name: expected $want_isa in the composed configuration"
    continue
  fi
  if ! "$MERGE" -o "$merged.again" "${layer_abs[@]}" "$target" || ! cmp -s "$merged" "$merged.again"; then
    problem "FAIL $name: composition is not deterministic"
    continue
  fi
  printf '  ok   %s\n' "$name"
done

# ---------------------------------------------------------------------------
# 3. Every symbol used by the layer tree must exist in Kconfig.
# ---------------------------------------------------------------------------
known="$tmp/known-symbols"
used="$tmp/used-symbols"
used_lines="$tmp/used-lines"

find "$NEMU_HOME" -name 'Kconfig*' -type f -not -path "$NEMU_HOME/tools/*" -print0 |
  xargs -0 awk '$1 == "config" || $1 == "menuconfig" { print $2 }' |
  sort -u >"$known"

grep -rHn -E '^(CONFIG_[A-Za-z0-9_]+=|# CONFIG_[A-Za-z0-9_]+ is not set$)' \
  "$CONFIGS" --include='*.config' --include='*_defconfig' >"$used_lines" || true
if [ -f "$CONFIGS/defconfig" ]; then
  grep -Hn -E '^(CONFIG_[A-Za-z0-9_]+=|# CONFIG_[A-Za-z0-9_]+ is not set$)' \
    "$CONFIGS/defconfig" >>"$used_lines" || true
fi

sed -E -e 's/^[^:]+:[0-9]+:CONFIG_([A-Za-z0-9_]+)=.*/\1/' \
       -e 's/^[^:]+:[0-9]+:# CONFIG_([A-Za-z0-9_]+) is not set$/\1/' \
       "$used_lines" | sort -u >"$used"

unknown="$(comm -13 "$known" "$used")"
if [ -n "$unknown" ]; then
  while IFS= read -r symbol; do
    problem "unknown symbol CONFIG_$symbol"
    grep -F "CONFIG_$symbol" "$used_lines" | sed 's/^/         /' >&2
  done <<<"$unknown"
fi

# ---------------------------------------------------------------------------
# 4. Report layers that no target uses.
# ---------------------------------------------------------------------------
: >"$tmp/used-layers"
for target in "${targets[@]}"; do
  while IFS= read -r layer; do
    realpath "$CONFIGS/$layer"
  done < <(sed -n 's|^# layer: ||p' "$target")
done | sort -u >"$tmp/used-layers"

while IFS= read -r layer; do
  if ! grep -qxF "$(realpath "$layer")" "$tmp/used-layers"; then
    printf '  warn layer is not used by any target: %s\n' "${layer#"$NEMU_HOME"/}" >&2
  fi
done < <(find "$CONFIGS" -mindepth 2 -name '*.config' -type f | sort)

# ---------------------------------------------------------------------------
# Summary.
# ---------------------------------------------------------------------------
layers=$(find "$CONFIGS" -mindepth 2 -name '*.config' -type f | wc -l)
if [ "$fail" -ne 0 ]; then
  printf '%sConfiguration check failed.%s\n' "$color_red" "$color_end" >&2
  exit 1
fi

printf '%sOK: %d targets, %d layers (%d override slots, %d redundant)%s\n' \
  "$color_green" "${#targets[@]}" "$layers" "$override_slots" "$redundant_slots" "$color_end"
