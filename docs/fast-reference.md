# Fast shared reference

Build one shared REF supporting FAST and SLOW:

```sh
make NEMU_HOME="$PWD" git_commit= riscv64-xs-fastref_defconfig
make NEMU_HOME="$PWD" git_commit= -j8
```

`FAST_REF` enables mode and checkpoint APIs. `PERF_OPT_SHARE` independently
enables the shared optimized interpreter with exact instruction counting.
`STORE_LOG_HASH` collects an ordered RAM-store digest; rollback logging remains
controlled by `STORE_LOG` and its existing runtime flag.

The default mode is SLOW. Select FAST with
`difftest_set_exec_mode(DIFFTEST_EXEC_FAST)`. FAST executes `difftest_exec(n)`
continuously, suppresses committed-store queue entries and bypasses PMP/PMA.
SLOW restores configured checks and executes requests one instruction at a
time. FAST results require independent SLOW validation; scheduling and packet
transport belong to DiffTest.

Apply skips and events at their instruction boundaries. Compare
`difftest_get_instr_count()` before and after execution to detect short runs.
Use `difftest_regcpy` explicitly when a state snapshot is needed. Change modes
at consumed boundaries: switching clears the committed-store queue and
refreshes derived MMU/permission state.

Additional APIs are exported only with `FAST_REF`:

| API (prefix `difftest_`) | Purpose |
| --- | --- |
| `set_exec_mode`, `get_instr_count`, `get_pc` | Select mode and query progress |
| `skip_one` | Advance PC and optionally supply integer register writeback |
| `exec_skip` | Execute one instruction, then supply integer writeback |
| `flush_state` | Refresh derived state before a forked worker resumes |
| `state_hash` | Return CPU-state and ordered store digests; 0 on success, -1 if store hashing is unavailable or paused |
| `set_store_hash`, `store_hash_reset` | Pause/resume collection or clear its digest |

Store hashing starts enabled when configured. Each scalar RAM write updates
two hash words and a record count, using aligned address/data/mask records.
It retains no additional store-record queue or old memory values. Forked REFs
inherit the same digest prefix and can compare at an agreed endpoint;
queries do not reset the digest. Rollback restores its matching digest state.
The hashes do not establish full-memory equality.
