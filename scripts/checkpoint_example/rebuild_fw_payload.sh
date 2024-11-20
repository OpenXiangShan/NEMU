#!/bin/bash

# usage: FW_PAYLOAD=/path/to/fw_payload.bin CROSS_COMPILE=/path/to/riscv64-unknown-linux-gnu- bash rebuild_fw_payload.sh
# get extract_fw_payload/build/gcpt.bin

REPO_LIST="repo.list"
WORKSPACE=$(pwd)

set -x

if [ ! -f "$REPO_LIST" ]; then
  echo ""
  exit 1
fi

while IFS= read -r REPO_URL || [[ -n "$REPO_URL" ]]; do
  REPO_NAME=$(basename "$REPO_URL" .git)

  if [ -d "$REPO_NAME" ]; then
    echo "Updating repository '$REPO_NAME'..."
    cd "$REPO_NAME" && git pull && cd ..
  else
    echo "Clone repo 'REPO_URL' into '$REPO_NAME'..."
    git clone "$REPO_URL"
  fi
done < "$REPO_LIST"

cd nemu_board/dts && bash build_single_core_for_nemu.sh && cd $WORKSPACE

export RESULT=extract_fw_payload

mkdir $RESULT

# note: default payload offset in opensbi is 2M, if you have modified FW_PAYLOAD_OFFSET, modify the next line of command to match this value
dd if=$FW_PAYLOAD of=$RESULT/kernel.Image bs=1M skip=2

kernel_list=extrack_fw_payload/kernel.Image

fw_payload_bin="$kernel_list"

fw_payload_bin_size=$(stat -c%s "$fw_payload_bin")

fw_payload_fdt_addr=$(( ( (fw_payload_bin_size + 0x800000 + 0xfffff) / 0x100000 ) * 0x100000 + 0x80000000 ))

printf "SPEC: %s, file size: %X, fw_payload_fdt_addr: %X\n" "$kernel_list" "$fw_payload_bin_size" "$fw_payload_fdt_addr"

make -C $(pwd)/opensbi/ O=$(pwd)/$RESULT/opensbi PLATFORM=generic FW_PAYLOAD_PATH="$(pwd)/$RESULT/kernel.Image" FW_FDT_PATH=$(pwd)/nemu_board/dts/build/xiangshan.dtb FW_PAYLOAD_OFFSET=0x100000 FW_PAYLOAD_FDT_ADDR=0x$(printf '%X' $fw_payload_fdt_addr) -j10

make -C $(pwd)/LibCheckpointAlpha/ O=$(pwd)/$RESULT GCPT_PAYLOAD_PATH="$(pwd)/$RESULT/opensbi/platform/generic/firmware/fw_payload.bin"



