# rm -rf ./redis_output_top/test_manual
mkdir -p ./redis_cpt
./build/riscv64-nemu-interpreter \
    --cpt-interval 10000000 -u -b \
    -D redis_cpt \
    -C 10M \
    -w get_loop \
    -r ./resource/gcpt_restore/build/gcpt.bin \
    --manual-uniform-cpt \
    /nfs/home/zhangchuanqi/lvna/for_xs/xs-env/sw/riscv-pk/build/bbl.bin
    # /nfs/home/zhangchuanqi/lvna/for_xs/xs-env/NEMU/ready-to-run/linux-0xa0000.bin