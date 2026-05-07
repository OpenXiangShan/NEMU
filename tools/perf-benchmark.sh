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

set -euo pipefail

export NEMU_HOME="${NEMU_HOME:-$PWD}"

defconfig=""
title=""
repo_dir=""
workloads="${CI_WORKLOADS:-./workloads}"
dynamorio_dir="DynamoRIO-Linux-11.3.0-1"
output="performance_results.md"
metrics_output=""
include_linux=true
binary=""
defconfig_patch=""
defconfig_gen_script=""
patched_defconfig=""
defconfig_backup=""
defconfig_backup_dir=""
prepared_defconfig_target=""

usage() {
  cat <<'EOF'
Usage: tools/perf-benchmark.sh --defconfig DEFCONFIG [options]

Build one NEMU defconfig, run the CI performance workload set, and emit a
markdown table for that single build.

Options:
  --defconfig NAME      Defconfig to build, such as riscv64-xs_defconfig.
  --title TITLE         Markdown section title. Defaults to the defconfig name.
  --workloads DIR       Workload directory. Defaults to $CI_WORKLOADS or ./workloads.
  --dynamorio-dir DIR   Extracted DynamoRIO directory.
  --output FILE         Markdown report path.
  --metrics-output FILE Optional TSV metrics output for CI comparison.
  --repo-dir DIR        NEMU repository directory. Defaults to the current dir.
  --binary FILE         Executable to benchmark. Defaults to build/riscv64-nemu-interpreter.
  --defconfig-patch FILE
                        Apply a patch to configs/DEFCONFIG before building.
  --defconfig-gen-script FILE
                        Generate configs/DEFCONFIG before building.
  --no-linux            Skip the linux/hello system workload.
  -h, --help            Show this help.
EOF
}

cleanup() {
  if [ -n "$defconfig_backup" ] && [ -n "$patched_defconfig" ] && [ -f "$defconfig_backup" ]; then
    cp "$defconfig_backup" "$patched_defconfig"
  elif [ -n "$patched_defconfig" ] && [ -f "$patched_defconfig" ]; then
    rm -f "$patched_defconfig"
  fi
  if [ -n "$defconfig_backup_dir" ] && [ -d "$defconfig_backup_dir" ]; then
    rm -rf "$defconfig_backup_dir"
  fi
}

require_value() {
  if [ "$#" -lt 2 ] || [ -z "$2" ]; then
    echo "Missing value for $1" >&2
    exit 2
  fi
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --defconfig)
      require_value "$@"
      defconfig="$2"
      shift 2
      ;;
    --title)
      require_value "$@"
      title="$2"
      shift 2
      ;;
    --workloads)
      require_value "$@"
      workloads="$2"
      shift 2
      ;;
    --dynamorio-dir)
      require_value "$@"
      dynamorio_dir="$2"
      shift 2
      ;;
    --output)
      require_value "$@"
      output="$2"
      shift 2
      ;;
    --metrics-output)
      require_value "$@"
      metrics_output="$2"
      shift 2
      ;;
    --repo-dir)
      require_value "$@"
      repo_dir="$2"
      shift 2
      ;;
    --binary)
      require_value "$@"
      binary="$2"
      shift 2
      ;;
    --defconfig-patch)
      require_value "$@"
      defconfig_patch="$2"
      shift 2
      ;;
    --defconfig-gen-script)
      require_value "$@"
      defconfig_gen_script="$2"
      shift 2
      ;;
    --no-linux)
      include_linux=false
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [ -z "$defconfig" ]; then
  echo "Missing required --defconfig" >&2
  usage >&2
  exit 2
fi

repo_dir="${repo_dir:-$PWD}"
repo_dir=$(realpath "$repo_dir")
workloads=$(realpath "$workloads")
dynamorio_dir=$(realpath "$dynamorio_dir")
output=$(realpath -m "$output")
if [ -n "$metrics_output" ]; then
  metrics_output=$(realpath -m "$metrics_output")
fi
if [ -n "$defconfig_patch" ]; then
  defconfig_patch=$(realpath "$defconfig_patch")
fi
if [ -n "$defconfig_gen_script" ]; then
  defconfig_gen_script=$(realpath "$defconfig_gen_script")
fi
export NEMU_HOME="$repo_dir"

if [ -z "$title" ]; then
  title="$defconfig"
fi

drrun="${dynamorio_dir}/bin64/drrun"
inscount="${dynamorio_dir}/samples/bin64/libinscount.so"

binary="${binary:-${repo_dir}/build/riscv64-nemu-interpreter}"
binary=$(realpath -m "$binary")

if [ ! -x "$drrun" ]; then
  echo "DynamoRIO drrun not found or not executable: $drrun" >&2
  exit 1
fi

if [ ! -f "$inscount" ]; then
  echo "DynamoRIO inscount client not found: $inscount" >&2
  exit 1
fi

if [ -n "$defconfig_patch" ] && [ ! -f "$defconfig_patch" ]; then
  echo "Defconfig patch not found: $defconfig_patch" >&2
  exit 1
fi

if [ -n "$defconfig_gen_script" ] && [ ! -x "$defconfig_gen_script" ]; then
  echo "Defconfig generation script not found or not executable: $defconfig_gen_script" >&2
  exit 1
fi

if [ -n "$defconfig_patch" ] && [ -n "$defconfig_gen_script" ]; then
  echo "Use either --defconfig-patch or --defconfig-gen-script, not both" >&2
  exit 1
fi

trap cleanup EXIT

tests=(
  am/misc-tests/bin/bitmanip.bin
  am/coremark/bin/coremark-riscv64-xs-rv64gc-o2.bin
  am/coremark/bin/coremark-riscv64-xs-rv64gc-o3.bin
  am/coremark/bin/coremark-riscv64-xs-rv64gcb-o3.bin
  am/misc-tests/bin/amtest-riscv64-xs.bin
  am/misc-tests/bin/aliastest-riscv64-xs.bin
  am/misc-tests/bin/softprefetchtest-riscv64-xs.bin
  am/misc-tests/bin/zacas-riscv64-xs.bin
)

configure_perf_flags() {
  sed -i 's/CONFIG_CC_NATIVE_ARCH=y/# CONFIG_CC_NATIVE_ARCH is not set/g' .config
  sed -i 's/CONFIG_CC_OPT_FLAGS=""/CONFIG_CC_OPT_FLAGS="-march=x86-64-v3 -mtune=generic -ftree-vectorize"/g' .config
}

prepare_existing_defconfig_update() {
  prepared_defconfig_target="$repo_dir/configs/$defconfig"

  if [ ! -f "$prepared_defconfig_target" ]; then
    echo "Defconfig not found: $prepared_defconfig_target" >&2
    exit 1
  fi

  if [ -z "$defconfig_backup" ]; then
    defconfig_backup_dir=$(mktemp -d)
    defconfig_backup="$defconfig_backup_dir/$(basename "$prepared_defconfig_target")"
    patched_defconfig="$prepared_defconfig_target"
    cp "$prepared_defconfig_target" "$defconfig_backup"
  fi
}

apply_defconfig_patch() {
  prepare_existing_defconfig_update
  if ! command -v patch >/dev/null 2>&1; then
    echo "patch command not found" >&2
    exit 1
  fi

  if ! patch --dry-run --silent -d "$repo_dir" -p1 < "$defconfig_patch" >/dev/null; then
    echo "Failed to apply defconfig patch: $defconfig_patch" >&2
    exit 1
  fi
  patch --batch --silent -d "$repo_dir" -p1 < "$defconfig_patch" >/dev/null
}

prepare_generated_defconfig() {
  prepared_defconfig_target="$repo_dir/configs/$defconfig"

  if [ -z "$patched_defconfig" ]; then
    patched_defconfig="$prepared_defconfig_target"
    if [ -f "$prepared_defconfig_target" ]; then
      defconfig_backup_dir=$(mktemp -d)
      defconfig_backup="$defconfig_backup_dir/$(basename "$prepared_defconfig_target")"
      cp "$prepared_defconfig_target" "$defconfig_backup"
    fi
  fi
}

apply_defconfig_gen_script() {
  prepare_generated_defconfig
  "$defconfig_gen_script" "$prepared_defconfig_target"
}

build_perf_binary() {
  make clean-all
  if [ -n "$defconfig_patch" ]; then
    apply_defconfig_patch
  fi
  if [ -n "$defconfig_gen_script" ]; then
    apply_defconfig_gen_script
  fi
  make "$defconfig"
  configure_perf_flags
  make -j "$(nproc)"
  if [ "$(basename "$binary")" = "runner" ] && [ -f "$(dirname "$binary")/runner.c" ]; then
    make -C "$(dirname "$binary")"
  fi
  if [ ! -x "$binary" ]; then
    echo "Benchmark binary not found or not executable: $binary" >&2
    exit 1
  fi
}

run_target() {
  local image="$1"
  shift

  "$@" "$binary" -b "$image"
}

get_sim_freq() {
  local line

  line=$(printf '%s\n' "$1" | grep "simulation frequency" | tail -n 1 || true)
  if [ -z "$line" ]; then
    return
  fi
  printf '%s\n' "${line#*=}" | tr -cd '0-9'
}

get_host_instr_count() {
  local line

  line=$(printf '%s\n' "$1" | grep "Instrumentation results:" | tail -n 1 || true)
  printf '%s\n' "$line" | tr -cd '0-9'
}

run_native_freq() {
  local image="$1"
  local native_output sim_freq

  native_output=$(run_target "$image" | strings)
  sim_freq=$(get_sim_freq "$native_output")
  if [ -n "$sim_freq" ]; then
    printf '%s\n' "$sim_freq"
  else
    printf 'N/A\n'
  fi
}

format_sci() {
  python3 -c 'import sys
value = sys.argv[1]
if value == "" or value == "N/A":
    print("N/A")
else:
    print(f"{float(value):.3e}")' "$1"
}

append_result_row() {
  local test_name="$1"
  local image="$2"
  local result guest_instr_count host_instr_count actual_throughput estimated_throughput

  echo "::group::${title} - ${test_name}"
  result=$(run_target "$image" "$drrun" -c "$inscount" -- | strings)
  guest_instr_count=$(echo "$result" | grep "total guest instructions" | tail -n 1 | cut -d '=' -f2 | tr -cd '0-9')
  host_instr_count=$(get_host_instr_count "$result")
  if [ -z "$guest_instr_count" ] || [ -z "$host_instr_count" ]; then
    echo "Failed to parse instruction counts for ${test_name}" >&2
    exit 1
  fi
  # Estimate host throughput with a fixed 4GHz CPU and IPC=2.5 model.
  estimated_throughput=$(python3 -c "print(4e9 * 2.5 / $host_instr_count * $guest_instr_count)")
  actual_throughput=$(run_native_freq "$image")
  printf '| %s | %s | %s | %s | %s |\n' \
    "$test_name" \
    "$(format_sci "$guest_instr_count")" \
    "$(format_sci "$host_instr_count")" \
    "$(format_sci "$estimated_throughput")" \
    "$(format_sci "$actual_throughput")" \
    >> "$output"
  if [ -n "$metrics_output" ]; then
    printf '%s\t%s\t%s\t%s\t%s\n' \
      "$test_name" \
      "$guest_instr_count" \
      "$host_instr_count" \
      "$estimated_throughput" \
      "$actual_throughput" \
      >> "$metrics_output"
  fi
  echo "::endgroup::"
}

cd "$repo_dir"

build_perf_binary

mkdir -p "$(dirname "$output")"
cat > "$output" <<EOF
### ${title}

Defconfig: \`${defconfig}\`

Binary: \`${binary}\`

| Test | Guest Instructions | Host Instructions | Estimated Host Throughput (instr/s) | Actual NEMU Throughput (instr/s) |
|------|--------------------|-------------------|-------------------------------------|----------------------------------|
EOF

if [ -n "$metrics_output" ]; then
  mkdir -p "$(dirname "$metrics_output")"
  printf 'test\tguest_instructions\thost_instructions\testimated_host_throughput\tactual_nemu_throughput\n' > "$metrics_output"
fi

for test_bin in "${tests[@]}"; do
  append_result_row "$(basename "$test_bin")" "${workloads}/${test_bin}"
done

if [ "$include_linux" = true ]; then
  append_result_row "linux-hello" "${workloads}/linux/hello/fw_payload.bin"
fi

cat >> "$output" <<'EOF'

* Host Instructions is measured by DynamoRIO's inscount client.
* Estimated Host Throughput assumes a fixed 4GHz CPU and IPC=2.5.
* Actual NEMU Throughput is a single native NEMU run and may vary with host CPU performance.
EOF
