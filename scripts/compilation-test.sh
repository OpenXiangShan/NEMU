#!/bin/bash

INFO="\033[1m"
ERROR="\033[1;31m"
GOOD="\033[1;32m"
NC="\033[0m"

failed_configs=()

if [ ! -f "$NEMU_HOME/src/nemu-main.c" ]; then
  echo -e "${ERROR}NEMU_HOME=$NEMU_HOME is not a NEMU repo${NC}"
  exit 1
fi

echo -e "${INFO}Start compilation test for all configurations${NC}"
make clean-all

for config_path in configs/riscv64-*defconfig; do
    if [[ -f "$config_path" ]]; then
        config=$(basename "$config_path")
        echo "::group::$config"
        echo -e "${INFO}Building configuration: $config${NC}"
        
        make $config
        if [ $? -ne 0 ]; then
            echo -e "${ERROR}Failed to setup configuration $config${NC}"
            failed_configs+=("$config")
            continue
        fi
        
        make -j8
        if [ $? -ne 0 ]; then
            echo -e "${ERROR}Compilation failed for configuration: $config${NC}"
            failed_configs+=("$config")
            continue
        fi

        echo -e "${GOOD}Successfully built: $config${NC}"
        echo "::endgroup::"
    else
        echo -e "${ERROR}No riscv64-*defconfig files found in configs directory.${NC}"
        exit 1
    fi
done

if [ ${#failed_configs[@]} -ne 0 ]; then
    echo -e "${ERROR}The following configurations failed to build:${NC}"
    for failed_config in "${failed_configs[@]}"; do
        echo "$failed_config"
    done
    exit 1
else
    echo -e "${GOOD}All configurations built successfully.${NC}"
    exit 0
fi
