#!/usr/bin/env bash
set -euo pipefail

CONFIGS_DIR="configs"

if [ ! -d "$CONFIGS_DIR" ]; then
  echo "Error: directory '$CONFIGS_DIR' does not exist."
  exit 1
fi

# Backup original .config
if [ -f .config ]; then
  cp .config .config.backup
  HAS_BACKUP=1
else
  HAS_BACKUP=0
fi

total=0
updated=0

for cfg in "$CONFIGS_DIR"/*_defconfig; do
  base=$(basename "$cfg")
  total=$((total + 1))

  # status line prefix
  printf "Processing: %s ... " "$base"

  make "$base" > /dev/null

  if diff -u "$cfg" .config > /dev/null; then
    echo "ok"
  else
    echo "failed"
    cp .config "$cfg"
    updated=$((updated + 1))
  fi
done

# Restore original .config
if [ "$HAS_BACKUP" -eq 1 ]; then
  mv .config.backup .config
else
  rm -f .config
fi

echo "Summary: $total processed, $updated updated."
