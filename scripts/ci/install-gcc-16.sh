#!/usr/bin/env bash

set -euo pipefail

required_major=16

check_compiler() {
  command -v gcc-16 >/dev/null 2>&1 &&
    command -v g++-16 >/dev/null 2>&1 &&
    [ "$(gcc-16 -dumpversion | cut -d. -f1)" = "$required_major" ] &&
    [ "$(g++-16 -dumpversion | cut -d. -f1)" = "$required_major" ]
}

if check_compiler; then
  gcc-16 --version | sed -n '1p'
  g++-16 --version | sed -n '1p'
  exit 0
fi

if ! command -v apt-get >/dev/null 2>&1; then
  echo "GCC 16 is required, but apt-get is unavailable" >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y --no-install-recommends ca-certificates gnupg software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install -y --no-install-recommends gcc-16 g++-16

if ! check_compiler; then
  echo "Failed to install a usable GCC 16 C/C++ toolchain" >&2
  exit 1
fi

gcc-16 --version | sed -n '1p'
g++-16 --version | sed -n '1p'
