cpt=`find output_top/test/linux/0 -name "*.gz"`
./build/riscv64-nemu-interpreter -b\
    -r resource/gcpt_restore/build/gcpt.bin \
    --restore -I 99000000\
    $cpt
