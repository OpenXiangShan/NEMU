./build/riscv64-nemu-interpreter \
    --cpt-interval 10000000 -u -b \
    -D output_top \
    -C test \
    -w linux \
    -r ./resource/gcpt_restore/build/gcpt.bin \
    --dont-skip-boot\
    -I 11000000 ./ready-to-run/linux-0xa0000.bin
