# NEMU Perf Profiling

`tools/perf_profile.py` collects host-side performance data for a NEMU run
with Linux `perf`. It keeps the raw `perf` outputs, parsed JSON artifacts, a
manifest, and Markdown reports in one run directory.

The framework answers three practical questions:

- How many host counters did one NEMU run consume?
- Which functions are hot?
- Which basic blocks are hot?

## Requirements

- Linux on an x86 host
- `perf`
- `addr2line`
- `nm`
- Python 3.10+
- a NEMU binary with symbols; debug info is recommended

## Get Started

Build NEMU first:

```bash
make riscv64-xs_defconfig
make -j
```

Run one quick profile with a fixed guest-instruction budget:

```bash
python3 tools/perf_profile.py run \
  --mode quick \
  --binary ./build/riscv64-nemu-interpreter \
  --workload ./workloads/linux/coremark-pro/fw_payload.bin \
  --nemu-args='-b -I 1000000000' \
  --output-root perf_profile_out
```

Open the generated summary report:

```text
perf_profile_out/<run-id>/reports/summary.md
```

Example summary output:

```text
# NEMU perf profile summary

## Run Summary
- mode: quick
- guest instructions: 1000001488
- simulation frequency: 185235783 instr/s

## Global Counters
| event | count | running_pct |
|---|---:|---:|
| cycles | 34623153083 | 99.9400 |
| instructions | 140751835752 | 99.9400 |

## Sample Counts
- grouped sample groups: 5207
| event | samples |
|---|---:|
| instructions | 5207 |
| branch-misses | 5207 |
| cache-misses | 2765 |

## Reports
- functions_quick: reports/functions_quick.md
- basic_blocks_quick: reports/basic_blocks_quick.md
```

`-b` by itself may run for a long time on Linux workloads because NEMU waits for
the guest workload to finish naturally. Use `-I <guest-instructions>` when you
want one bounded profiling window that exits quickly.

With the default example above, users should expect one quick profiling run to
finish in tens of seconds on a typical development machine, not in several
minutes. If it ends almost immediately, or still runs much longer than you
expect, adjust `-I` for your host and workload.

## Workload Images

The examples in this document use workload images such as
`./workloads/linux/coremark-pro/fw_payload.bin`.

NEMU does not include the workload builder as a git submodule. Instead, the
workload builder is an external example repository that users can fetch and use
separately:

- repository: <https://github.com/OpenXiangShan/workload-builder>

This profiling framework only consumes the generated workload images. It does
not depend on the workload builder being checked out inside the NEMU tree.

## Recommended Windows

For Linux workloads, treat the instruction budget as a profiling window. The
best default is not the shortest run that exits, but the shortest run that
still gives stable samples on your machine.

Recommended starting points:

- smoke check: `-I 10000000`
  - use this only to verify that the toolchain and command line work
- short exploratory window: `-I 100000000`
  - use this when you want a fast first look
- default quick profile: `-I 1000000000`
  - use this as the default first profile for Linux workloads when you want a
    more stable hotspot ranking
  - expect the full quick profiling flow to finish in tens of seconds
- longer comparison window: `-I 4000000000` to `-I 10000000000`
  - use this when repeated runs still move around too much, or when you want a
    stronger steady-state signal
  - expect the full quick profiling flow to take noticeably longer, up to
    minutes on slower hosts

These timing expectations are only rough guidance. The right window is
host-dependent, and the real test is whether the resulting sample counts and
hotspot ordering are stable enough for your task.

If the summary report still shows low-sample warnings or the hotspot order
changes noticeably across repeated runs, increase `-I` and keep it fixed for
round-to-round comparisons.

If the environment is not ready yet, run `doctor` first:

```bash
python3 tools/perf_profile.py doctor \
  --binary ./build/riscv64-nemu-interpreter \
  --workload ./workloads/linux/coremark-pro/fw_payload.bin
```

## Profiling Modes

### Quick Mode

Quick mode is the default starting point. It runs:

- one `perf stat`
- one grouped `perf record`

The grouped sampling event list is:

```text
{instructions,cycles,branches,branch-misses,cache-references,cache-misses}:S
```

`instructions` is the only trigger event in quick mode. The other counters are
grouped readouts attached to the same samples. Treat them as auxiliary metrics,
not as independent event-triggered hotspot rankings.

For Linux workloads, prefer a bounded command such as
`--nemu-args='-b -I 1000000000'` for the first pass. Natural completion with
bare `-b` can take much longer.

### Precise Mode

Precise mode records each sampled event in a separate NEMU run:

```bash
python3 tools/perf_profile.py run \
  --mode precise \
  --binary ./build/riscv64-nemu-interpreter \
  --workload ./workloads/linux/coremark-pro/fw_payload.bin \
  --nemu-args='-b -I 1000000000' \
  --precise-events instructions,cycles,branch-misses,cache-misses
```

Use this mode when you need true event-triggered attribution for a specific
counter and can afford the extra runtime.

## Subcommands

- `doctor`: check the host tools, kernel settings, binary, and workload
- `stat`: collect only global `perf stat` counters
- `record`: collect only sampling data
- `report`: regenerate reports from an existing run directory
- `run`: execute `doctor -> stat -> record -> report`

## Run Layout

Each `run` directory contains:

```text
manifest.json
logs/
raw/
parsed/
reports/
```

Important outputs:

- `reports/summary.md`: run configuration, throughput, global counters,
  sample counts, warnings, and report links
- `reports/functions_quick.md`: function hotspots for quick mode
- `reports/basic_blocks_quick.md`: basic-block hotspots for quick mode
- `reports/functions_<event_id>.md`: function hotspots for precise mode
- `reports/basic_blocks_<event_id>.md`: basic-block hotspots for precise mode
- `reports/doctor.md`: environment checks
- `parsed/stat.json`: parsed `perf stat` counters
- `parsed/quick_samples.json`: parsed grouped samples in quick mode
- `parsed/quick_functions.json`: aggregated quick-mode function hotspots
- `parsed/basic_blocks_quick.json`: aggregated quick-mode basic-block hotspots
- `parsed/samples_<event_id>.json`: parsed samples for precise mode
- `parsed/functions_<event_id>.json`: aggregated precise-mode function hotspots
- `parsed/basic_blocks_<event_id>.json`: aggregated precise-mode basic-block
  hotspots

Here `<event_id>` is the filename-safe identifier derived from the requested
event string. For plain events such as `instructions` or `cycles`, the filename
stays readable, for example `reports/functions_instructions.md`. Events with
modifiers or raw PMU syntax are sanitized for filesystem use, and some forms
may also gain a short hash suffix to avoid collisions.

## Reading the Reports

- Start with `reports/summary.md` to confirm the command line, throughput, and
  global counters.
- Check the sample-count section in `reports/summary.md` before trusting small
  hotspot differences between runs.
- Use quick mode to find the dominant instruction-triggered hotspots first.
- Switch to precise mode only when you need event-specific attribution for
  `cycles`, `branch-misses`, `cache-misses`, or another sampled counter.
