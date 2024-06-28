#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

failed_configs=()

for config_path in configs/riscv64-*defconfig; do
    if [[ -f "$config_path" ]]; then
        config=$(basename "$config_path")
        echo "Building configuration: $config"

        make clean
        
        # 执行 make 命令
        make "$config"
        if [ $? -ne 0 ]; then
            echo "Failed to execute 'make $config'"
            exit 1
        fi
        
        make -j8
        if [ $? -ne 0 ]; then
            echo "Compilation failed for configuration: $config"
            failed_configs+=("$config")
            continue
        fi

        echo "Successfully built: $config"
    else
        echo "No riscv64-*defconfig files found in configs directory."
        exit 1
    fi
done

if [ ${#failed_configs[@]} -ne 0 ]; then
    echo "The following configurations failed to build:"
    for failed_config in "${failed_configs[@]}"; do
        echo "$failed_config"
    done
    exit 1
else
    echo "All configurations built successfully."
    exit 0
fi
