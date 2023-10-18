./build/riscv64-nemu-interpreter \
    --cpt-interval 100000000 -u -b \
    -D output_top \
    -C test_manual \
    -w linux \
    -r ./resource/gcpt_restore/build/gcpt.bin \
    --manual-uniform-cpt \
    $n/projects/rv-linux/riscv-pk/bbl-debian.bin
