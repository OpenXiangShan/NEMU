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

# Regression tests for Kconfig's effective-value checks, using a tiny tree.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nemu-check-test.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
# The fixture must never write caller-selected configuration outputs.
unset KCONFIG_CONFIG KCONFIG_AUTOCONFIG KCONFIG_AUTOHEADER KCONFIG_TRISTATE
# make command-line overrides can otherwise restore those paths in child make.
unset MAKEFLAGS MAKEOVERRIDES MFLAGS
mkdir -p "$tmp/scripts" "$tmp/configs" "$tmp/tools/kconfig/build" "$tmp/tools/fixdep/build"
cp "$here/"{check_configs,merge_config,merge_target}.sh "$tmp/scripts/"
ln -s "$here/../tools/kconfig/build/conf" "$tmp/tools/kconfig/build/conf"
ln -s "$here/../tools/fixdep/build/fixdep" "$tmp/tools/fixdep/build/fixdep"
printf 'all:;@:\n' | tee "$tmp/tools/kconfig/Makefile" >"$tmp/tools/fixdep/Makefile"
cat >"$tmp/Kconfig" <<'EOF'
config ISA_riscv64
  bool "riscv64"
config PARENT
  bool "parent"
config CHILD
  bool "child"
  depends on PARENT
config ADDRESS
  hex "address"
  depends on PARENT
config LABEL
  string "label"
  depends on PARENT
config COUNT
  int "count"
  default 1
  range 1 4
config FORCED
  bool
  default y
EOF

check() {
  local name="$1" status="$2" diagnostic="$3" settings="$4" got=0
  printf 'CONFIG_ISA_riscv64=y\n%s\n' "$settings" >"$tmp/configs/riscv64-test_defconfig"
  NEMU_HOME="$tmp" bash "$tmp/scripts/check_configs.sh" >"$tmp/log" 2>&1 || got=$?
  if { [ "$status" = pass ] && [ "$got" -ne 0 ]; } ||
     { [ "$status" = fail ] && [ "$got" -eq 0 ]; } ||
     ! grep -qF "$diagnostic" "$tmp/log"; then
    echo "FAIL $name" >&2
    cat "$tmp/log" >&2
    exit 1
  fi
  echo "  ok   $name"
}

check "active values" pass 'OK:' \
  $'CONFIG_PARENT=y\nCONFIG_CHILD=y\nCONFIG_ADDRESS=0x1234\nCONFIG_LABEL="a=b c"'
check "unavailable enabled option" fail 'CONFIG_CHILD requested y' \
  $'# CONFIG_PARENT is not set\nCONFIG_CHILD=y'
check "noncanonical boolean cannot masquerade as a scalar" fail 'CONFIG_CHILD requested yes' \
  $'# CONFIG_PARENT is not set\nCONFIG_CHILD=yes'
check "forced option cannot be disabled" fail 'CONFIG_FORCED requested n' \
  '# CONFIG_FORCED is not set'
check "absent n remains disabled" pass 'OK:' \
  $'# CONFIG_PARENT is not set\nCONFIG_CHILD=n'
check "inactive device parameters are visible" pass 'dormant: CONFIG_ADDRESS' \
  $'# CONFIG_PARENT is not set\n# CONFIG_CHILD is not set\nCONFIG_ADDRESS=0x1234'
check "inactive strings are visible" pass 'dormant: CONFIG_LABEL' \
  $'# CONFIG_PARENT is not set\nCONFIG_LABEL="a=b c"'
check "changed scalar values fail" fail 'CONFIG_COUNT requested 8' \
  'CONFIG_COUNT=8'
check "unknown symbols still fail" fail 'unknown symbol CONFIG_MISSING' \
  'CONFIG_MISSING=y'

# Exercise both ordinary named targets and the documented save/load round trip.
(
  cd "$tmp"
  export NEMU_HOME="$tmp"
  config_make="$here/config.mk"
  printf 'CONFIG_ISA_riscv64=y\nCONFIG_PARENT=y\nCONFIG_ADDRESS=0x1234\n' \
    >configs/riscv64-test_defconfig
  make -s -f "$config_make" riscv64-test_defconfig >"$tmp/make.log" 2>&1 || exit 1
  cp .config "$tmp/expected.config" || exit 1
  make -s -f "$config_make" savedefconfig >>"$tmp/make.log" 2>&1 || exit 1
  printf 'CONFIG_ISA_riscv64=y\n' >.config
  make -s -f "$config_make" defconfig >>"$tmp/make.log" 2>&1 || exit 1
  cmp -s .config "$tmp/expected.config"
) || { cat "$tmp/make.log" >&2; echo 'FAIL defconfig save/load round trip' >&2; exit 1; }
echo '  ok   ordinary make target and save/load round trip'
echo 'check_configs_test: OK'
