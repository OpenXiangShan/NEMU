#!/usr/bin/env bash
docker build -t difftest-so-env ./scripts && \
docker run --rm --volume=$(pwd):/root/NEMU --workdir /root/NEMU difftest-so-env bash -l scripts/generate_so_for_difftest.sh
