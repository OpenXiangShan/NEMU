# NEMU Software Architecture For Agents

This document gives AI agents a stable map of NEMU before editing code. Use it to choose the right module, keep commits narrow, and pick validation that matches the changed path.

## Top-Level Shape

NEMU has two major runtime shapes:

- **Standalone emulator**: `src/nemu-main.c` calls `init_monitor()` and then `engine_start()`. This path boots images, runs devices, can take or restore checkpoints, and can run DiffTest against Spike or another reference.
- **Reference shared object**: `CONFIG_SHARE` builds `riscv64-nemu-interpreter-so` for external DUTs. The exported DiffTest ABI is implemented through CPU and ISA reference code, especially `src/cpu/difftest/ref.c` and `src/isa/riscv64/difftest/ref.c`.

Most changes should be classified by the module that owns the state being changed, not by the file extension or call site.

## Startup Flow

Standalone execution follows this control flow:

```text
src/nemu-main.c
  -> init_monitor(argc, argv)
     -> parse CLI flags
     -> init_log()
     -> init_mem()
     -> init_isa()
     -> init_device()
     -> fill_memory(image, flash, restorer)
     -> init_difftest(ref_so, image_size, flash_size, port)
     -> init_regex(), init_wp_pool(), init_aligncheck()
  -> engine_start()
     -> cpu_exec()
```

Important files:

- `src/monitor/monitor.c`: command-line options, monitor initialization, DiffTest initialization point.
- `src/monitor/image_loader.c`: raw, gz, zstd image and checkpoint loading.
- `src/memory/paddr.c`: physical memory allocation and host/guest address mapping.
- `src/isa/riscv64/init.c`: RISC-V reset and ISA initialization.
- `src/device/device.c`: device initialization fan-out.

## Execution Flow

The CPU execution loop is centered in `src/cpu/cpu-exec.c`:

```text
cpu_exec(n)
  -> execute loop
     -> fetch/decode/execute through ISA-specific helpers
     -> update instruction count
     -> run DiffTest step when enabled
     -> handle traps, exceptions, longjmp exits
```

Key state:

- `CPU_state cpu`: global architectural state.
- `nemu_state`: run status and trap result.
- instruction counters from `get_abs_instr_count()` and `get_abs_instr_count_csr()`.
- `context_stack`: longjmp contexts used for execution exits and exceptions.

Execution changes are usually `cpu-core` unless they touch ISA-specific decode, CSR, MMU, interrupt, or vector behavior.

## RISC-V ISA Flow

RISC-V code lives under `src/isa/riscv64/` and should be treated as the `riscv64-isa` module.

Important subareas:

- `instr/`: decode and instruction helpers.
- `system/priv.c`: privileged CSRs and mode behavior.
- `system/mmu.c`: address translation and memory access permission checks.
- `system/intr.c`: traps and interrupt routing.
- `system/aia.c`: AIA/IMSIC-related interrupt state.
- `system/trigger.c`: trigger and debug-related behavior.
- `difftest/`: RISC-V architectural state copy and compare.
- `include/isa-def.h`: `CPU_state` layout and DiffTest-synchronized region.

Do not casually change the order or layout of DiffTest-synchronized CPU state. If a state-layout change is required, document how ref and DUT copy/check paths stay compatible.

## DiffTest Architecture

NEMU has two DiffTest roles:

- **DUT mode**: NEMU runs the workload and compares against a reference shared object, usually Spike.
- **Reference mode**: NEMU is compiled as a shared object and is driven by XiangShan, GEM5, or a runner.

DUT-mode path:

```text
monitor init
  -> init_difftest(ref_so)
     -> dlopen ref shared object
     -> resolve difftest_memcpy/regcpy/exec/raise_intr
     -> copy image and register state to ref

cpu_exec loop
  -> difftest_step(pc, npc)
     -> ref_difftest_exec(1)
     -> ref_difftest_regcpy(..., DIFFTEST_TO_DUT)
     -> isa_difftest_checkregs()
```

Reference-mode path:

```text
external DUT or ref-so-runner
  -> difftest_memcpy()
  -> difftest_regcpy()
  -> difftest_exec(n)
  -> difftest_raise_intr()
```

Important files:

- `include/cpu/difftest.h`: common DiffTest declarations and register/store check helpers.
- `src/cpu/difftest/dut.c`: DUT-mode reference loading and step control.
- `src/cpu/difftest/ref.c`: reference shared-object ABI surface.
- `src/isa/riscv64/difftest/dut.c`: RISC-V register compare.
- `src/isa/riscv64/difftest/ref.c`: RISC-V register copy and reference-side helpers.
- `tools/ref-so-runner/`: standalone runner for NEMU-as-reference shared object.

DiffTest changes often cross CPU and RISC-V paths. Keep commits narrow: generic ABI/control changes belong in `cpu-core` or `difftest`; RISC-V architectural state copy/check changes belong in `riscv64-isa`.

## Memory And Image Loading

Memory ownership is split as:

- `src/memory/`, `include/memory/`: host memory, physical memory, virtual memory, sparse memory, store queue wrappers.
- `src/monitor/image_loader.c`: loading images/checkpoints into memory.
- `src/isa/riscv64/system/mmu.c`: RISC-V address translation and permission behavior.

Image and checkpoint loading bugs can look like memory bugs. Classify by the failing invariant:

- decompression, file format, load size: `monitor-loader`
- guest physical memory allocation or host address mapping: `memory`
- RISC-V translation or permission: `riscv64-isa`

## Checkpoint Architecture

Checkpoint support spans several modules:

- `src/checkpoint/`, `include/checkpoint/`: serializer, path management, simpoint and semantic checkpoint metadata.
- `src/profiling/`: profiling state that can drive checkpoint timing.
- `resource/gcpt_restore/`: GCPT restorer program.
- `scripts/take_zstd.sh`, `scripts/restore_zstd.sh`: smoke flows.
- `scripts/checkpoint_example/`: user-facing checkpoint examples.
- `src/monitor/monitor.c`: CLI flags and mode selection.
- `src/monitor/image_loader.c`: restore image loading.

Checkpoint restore depends on matching the checkpoint corpus and restorer. Do not assume one global restorer is valid for every checkpoint corpus.

## Device Architecture

Device code is under `src/device/` and `include/device/`.

Important areas:

- `src/device/device.c`: device initialization.
- `src/device/io/`: MMIO and port IO map handling.
- `src/device/plic.c`, `src/isa/riscv64/clint.c`: interrupt-related device state.
- UART, disk, flash, timer, keyboard, VGA, audio device files.

Device changes can affect Linux boot and DiffTest determinism. For execution-sensitive device changes, the delivery standard requires the Linux DiffTest smoke unless blocked.

## Build, Config, And CI

Build/config ownership:

- `configs/`: checked-in defconfigs.
- `Kconfig` and nested `Kconfig` files: option definitions.
- `scripts/*.mk`: build fragments.
- `include/generated/` and `include/config/`: generated, not normally committed.

CI/harness ownership:

- `.github/workflows/`: GitHub Actions jobs.
- `scripts/init-ready-to-run.py`: ready-to-run artifact setup.
- runner scripts under `scripts/`.
- `tools/ref-so-runner/`.
- `agent/`: agent-facing guidance.

When changing configs, run `NEMU_HOME=$PWD bash ./scripts/bump_configs.sh` when practical and report if it was not run.

## Module-To-Validation Guide

- **ci-harness**: `git diff --check`, syntax checks for changed scripts, focused dry-run/smoke when available.
- **build-config**: target defconfig, build, and `scripts/bump_configs.sh`.
- **monitor-loader**: image/checkpoint load smoke and Linux DiffTest smoke.
- **cpu-core**: build plus Linux DiffTest smoke.
- **riscv64-isa**: build plus Linux DiffTest smoke; add focused workload for CSR/MMU/vector/interrupt if the change is local to one feature.
- **memory**: build plus Linux DiffTest smoke; checkpoint or large-image smoke if load/size behavior changes.
- **device**: Linux boot or device-specific workload plus Linux DiffTest smoke.
- **checkpoint**: take/restore smoke when possible, plus Linux DiffTest smoke if execution behavior changes.
- **docs-only**: `git diff --check`; no execution smoke is required unless examples or commands are generated and should be validated.

## Agent Editing Rules

1. Identify the module before editing.
2. Keep each commit inside one module boundary.
3. If a change appears to require multiple modules, split it or explain the exception in `Constraint:`.
4. Prefer existing NEMU configuration, Makefile, and script patterns.
5. Verification claims must name exact commands and results.
