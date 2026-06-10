---
name: nemu-harness
description: Maintain NEMU automation and verification harnesses, including DiffTest runner scripts, ready-to-run setup, checkpoint restore flows, CI workload coverage, and AI-agent-safe maintenance workflows for this repository.
---

# NEMU Harness

Use this skill when changing NEMU automation surfaces rather than ISA behavior itself:

- GitHub Actions, workload runners, or helper scripts
- `init-ready-to-run`, `ready-to-run`, Spike/NEMU reference setup
- DiffTest harness code, ref shared-object runner, checkpoint restore scripts
- agent-facing maintenance instructions for this repository

## Operating Rules

1. Start from the current checkout, not memory. Inspect branch, dirty files, and relevant scripts before editing.
2. Keep harness changes split by review topic: dependency/auth setup, runner structure, parallelism/logging, and new workload coverage should be separate commits or stacked PRs.
3. Do not mix emulator behavior fixes with harness-only changes unless the failing harness exposes a real NEMU bug and the user asked to fix both.
4. Preserve reproducibility: every random workload selection needs an explicit seed, manifest, and enough log output to identify the failing case.
5. Prefer repo scripts and defconfigs over ad hoc shell fragments in workflows.

## File Map

- `.github/workflows/ci.yml`: required CI jobs for build, basic boot, DiffTest, performance, and workload coverage.
- `.github/workflows/build.yml`: shared-object build checks.
- `scripts/init-ready-to-run.py`: fetches ready-to-run artifacts and GitHub release metadata.
- `scripts/ready-to-run.mk`: Makefile integration for ready-to-run setup.
- `scripts/bump_configs.sh`: canonical config refresh check used by CI.
- `scripts/compilation-test.sh`: all-config compilation check.
- `tools/ref-so-runner/`: minimal host-side runner for `riscv64-nemu-interpreter-so`.
- `docs/ref_so_runner.md`: documented ref shared-object runner behavior.
- `include/cpu/difftest.h`: shared DiffTest declarations.
- `src/cpu/difftest/dut.c`: DUT-side DiffTest control.
- `src/cpu/difftest/ref.c`: reference-side DiffTest dynamic loading and calls.
- `src/monitor/monitor.c`: CLI flags, image loading, checkpoint options, DiffTest initialization.
- `src/isa/riscv64/difftest/dut.c`: RISC-V architectural state comparison.
- `src/isa/riscv64/difftest/ref.c`: RISC-V reference shared-object API implementation.
- `src/isa/riscv64/include/isa-def.h`: CPU state layout; the DiffTest synchronized region is marked in comments.
- `resource/gcpt_restore/`: GCPT restore program source.
- `scripts/take_zstd.sh`, `scripts/restore_zstd.sh`: zstd checkpoint smoke flows.
- `scripts/checkpoint_example/`: checkpoint and profiling examples.
- `src/checkpoint/`: checkpoint serializer and metadata code.
- `src/monitor/image_loader.c`: image/checkpoint loading and decompression limits.

## Module Boundaries

- **build-config**: `Makefile`, `Kconfig`, `configs/`, `scripts/*.mk`.
- **ci-harness**: `.github/`, runner scripts under `scripts/`, `tools/ref-so-runner/`, `agent/`.
- **monitor-loader**: `src/monitor/`.
- **cpu-core**: `src/cpu/`, `include/cpu/`.
- **riscv64-isa**: `src/isa/riscv64/`.
- **memory**: `src/memory/`, `include/memory/`.
- **device**: `src/device/`, `include/device/`.
- **checkpoint**: `src/checkpoint/`, `include/checkpoint/`, `resource/gcpt_restore/`, checkpoint scripts.
- **profiling**: `src/profiling/`, `include/profiling/`, profiling tools/docs.
- **docs-only**: documentation and agent guidance.

One commit should stay inside one module boundary unless it is a direct interface+implementation pair, a same-change test/CI update, or a pure mechanical move.

## Standard Workflow

1. Discover current state:
   - `git status --short --branch`
   - inspect `.github/workflows/*.yml`, relevant `scripts/*.sh`, `scripts/*.py`, and `docs/*.md`
   - search with `grep -RIn` if `rg` is unavailable
2. Edit narrowly:
   - use `apply_patch`
   - keep script options documented in `usage()`
   - keep CI env names explicit and stable
3. Verify in layers:
   - shell scripts: `bash -n <script>`
   - Python scripts: `python3 -m py_compile <script>`
   - workflow/script diffs: `git diff --check`
   - config changes: `NEMU_HOME=$PWD make <defconfig>` and `NEMU_HOME=$PWD bash ./scripts/bump_configs.sh`
   - runner behavior: use dry-run or small local smoke before full workload jobs
4. Report what was actually run and what remains untested.

## Harness Boundaries

- NEMU-as-DUT with Spike uses `./build/riscv64-nemu-interpreter -b --diff <spike-so> <image>`.
- NEMU-as-reference shared object is `./build/riscv64-nemu-interpreter-so`.
- Checkpoint restore tests must use the matching restorer for the checkpoint corpus when one exists.
- Required CI should avoid private machine assumptions unless the job is explicitly pinned to a self-hosted runner and guarded for fork PRs.
