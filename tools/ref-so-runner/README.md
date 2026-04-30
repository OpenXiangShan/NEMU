# NEMU Ref Shared Object Runner

This wrapper exposes a minimal NEMU-compatible CLI and loads
`$NEMU_HOME/build/riscv64-nemu-interpreter-so` by default. It drives the shared
object through the difftest reference APIs exported by `src/cpu/difftest/ref.c`.

It is intended for host-side performance benchmarking of the ref-mode shared
object without changing the normal NEMU build flow. Generate a dedicated
performance defconfig before building so the shared reference can drive Linux
standalone without modifying `riscv64-xs-ref_defconfig`:

```sh
tools/ref-so-runner/gen-xs-ref-perf-defconfig.sh \
  configs/riscv64-xs-ref-perf_defconfig
make riscv64-xs-ref-perf_defconfig
make -j"$(nproc)"
make -C tools/ref-so-runner

tools/ref-so-runner/runner \
  -b ./workloads/linux/hello/fw_payload.bin
```

The runner loads the image at `0x80000000`, executes through repeated
`difftest_exec(1)` calls, initializes the built-in flash reset stub for
`CONFIG_RESET_FROM_MMIO`, and stops when NEMU reaches a good or bad trap.
`tools/perf-benchmark.sh` can generate the same performance defconfig
automatically.
