#***************************************************************************************
# Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#***************************************************************************************/

from __future__ import annotations

import re
from collections.abc import Iterable
from bisect import bisect_right
from collections import defaultdict
from pathlib import Path

from .catalog import DEFAULT_ANNOTATE_SYMBOL_LIMIT
from .models import SampleRecord
from .util import canonical_event, run_cmd


SAMPLE_RE = re.compile(
    r"^\s*(?P<comm>\S+)\s+"
    r"(?:(?P<pidtid>\d+/\d+)|(?P<pid>\d+)\s+(?P<tid>\d+))\s+"
    r"(?P<time>[0-9.]+):\s+"
    r"(?P<period>\d+)\s+"
    r"(?P<event>[^:]+):"
    r"(?:\s+(?P<ip>[0-9a-fA-Fx]+)\s+(?P<sym>\S+)\s+\((?P<dso>[^)]*)\))?\s*$"
)
CC_ADDR_RE = re.compile(r"^\s*(?P<addr>[0-9a-fA-Fx]+)\s+(?P<sym>\S+)\s+\((?P<dso>[^)]*)\)\s*$")
STAT_LINE_RE = re.compile(
    r"^\s*(?P<count>[0-9,]+)\s+(?P<event>\S+)(?P<rest>.*)$"
)
RUNNING_RE = re.compile(r"\((?P<pct>[0-9.]+)%\)")
ELAPSED_RE = re.compile(r"([0-9]+(?:\.[0-9]+)?)\s+seconds\s+time\s+elapsed")
ANN_INSN_RE = re.compile(r"^\s*(?P<pct>[0-9]+(?:\.[0-9]+)?)\s*:\s*(?P<addr>[0-9a-fA-F]+):\s*(?P<asm>.+)$")
ANN_EVENT_HDR_RE = re.compile(r"Disassembly of .* for (?P<pmu>[^\s]+) \((?P<samples>\d+) samples")
ANN_GROUP_HDR_RE = re.compile(r"anon group\s*\{(?P<events>[^}]*)\}")
ANN_GROUP_INSN_RE = re.compile(
    r"^\s*(?P<pcts>(?:[0-9]+(?:\.[0-9]+)?\s+){2,}):\s*(?P<addr>[0-9a-fA-F]+):\s*(?P<asm>.+)$"
)
NM_LINE_RE = re.compile(r"^(?P<addr>[0-9a-fA-F]+)\s+\S\s+(?P<sym>\S+)$")
TARGET_ADDR_RE = re.compile(r"\b(?:0x)?(?P<addr>[0-9a-fA-F]{4,})\b")

_SYMBOL_CACHE: dict[str, tuple[list[int], list[str]]] = {}
_ANNOTATE_CACHE: dict[tuple[str, str], str] = {}
CONTROL_TRANSFER_PREFIXES = (
    "j",
    "call",
    "ret",
    "syscall",
    "sysenter",
    "int",
    "iret",
)


def parse_perf_stat(stderr_text: str) -> dict[str, object]:
    counts: dict[str, int] = defaultdict(int)
    running_pct: dict[str, float] = {}
    unsupported: list[str] = []
    warnings: list[dict[str, str]] = []

    for line in stderr_text.splitlines():
        if "<not supported>" in line:
            unsupported.append(line.strip())
            continue
        match = STAT_LINE_RE.match(line)
        if not match:
            continue
        event = canonical_event(match.group("event"))
        try:
            count = int(match.group("count").replace(",", ""))
        except ValueError:
            continue
        counts[event] += count
        pct_match = RUNNING_RE.search(match.group("rest"))
        if pct_match:
            pct = float(pct_match.group("pct"))
            running_pct[event] = max(running_pct.get(event, 0.0), pct)

    elapsed = None
    elapsed_match = ELAPSED_RE.search(stderr_text)
    if elapsed_match:
        elapsed = float(elapsed_match.group(1))

    for event, pct in running_pct.items():
        if pct < 99.0:
            warnings.append(
                {
                    "code": "stat_multiplexed",
                    "message": f"{event} ran for {pct:.2f}% of the measurement window; perf stat scaling is in effect.",
                    "severity": "warning",
                }
            )

    return {
        "counts": dict(counts),
        "running_pct": running_pct,
        "elapsed_seconds": elapsed,
        "unsupported_lines": unsupported,
        "warnings": warnings,
    }


def parse_script_samples(
    script_lines: Iterable[str] | str,
    binary_name: str,
    include_external: bool = False,
) -> list[SampleRecord]:
    records: list[SampleRecord] = []
    line_iter = iter(script_lines.splitlines() if isinstance(script_lines, str) else script_lines)
    pending_line: str | None = None
    while True:
        line = pending_line
        pending_line = None
        if line is None:
            try:
                line = next(line_iter)
            except StopIteration:
                break

        match = SAMPLE_RE.match(line)
        if not match:
            continue

        event = canonical_event(match.group("event"))
        period = float(match.group("period"))
        command = match.group("comm")
        pid_tid = match.group("pidtid") or f"{match.group('pid')}/{match.group('tid')}"
        time = match.group("time")
        group_id = f"{command}|{pid_tid}|{time}"

        leaf = ""
        leaf_dso = ""
        leaf_ip = None
        chain: list[str] = []

        if match.group("sym"):
            leaf = match.group("sym").split("+0x", 1)[0].strip()
            leaf_dso = match.group("dso") or ""
            try:
                leaf_ip = int(match.group("ip"), 16) if match.group("ip") else None
            except ValueError:
                leaf_ip = None
            in_target_binary = binary_name in leaf_dso
            if leaf and leaf != "[unknown]" and (include_external or in_target_binary):
                chain.append(leaf)

        while True:
            try:
                callchain_line = next(line_iter)
            except StopIteration:
                callchain_line = None
            if callchain_line is None:
                break

            addr_match = CC_ADDR_RE.match(callchain_line)
            if not addr_match:
                pending_line = callchain_line
                break
            sym = addr_match.group("sym").split("+0x", 1)[0].strip()
            dso = addr_match.group("dso")
            if not leaf:
                leaf = sym
                leaf_dso = dso
                try:
                    leaf_ip = int(addr_match.group("addr"), 16)
                except ValueError:
                    leaf_ip = None
            in_target_binary = binary_name in (dso or "")
            if sym and sym != "[unknown]" and sym not in chain and (include_external or in_target_binary):
                chain.append(sym)

        if not leaf or leaf == "[unknown]":
            continue
        if not include_external and binary_name not in (leaf_dso or ""):
            continue

        records.append(
            SampleRecord(
                event=event,
                period=period,
                leaf=leaf,
                leaf_dso=leaf_dso,
                ip=leaf_ip,
                chain=chain or [leaf],
                group_id=group_id,
                time=time,
                command=command,
                pid_tid=pid_tid,
            )
        )
    return records


def _load_symbol_table(binary: Path) -> tuple[list[int], list[str]]:
    key = str(binary.resolve())
    cached = _SYMBOL_CACHE.get(key)
    if cached is not None:
        return cached

    proc = run_cmd(["nm", "-n", "--defined-only", str(binary)], check=False)
    addrs: list[int] = []
    symbols: list[str] = []
    for line in proc.stdout.splitlines():
        match = NM_LINE_RE.match(line.strip())
        if not match:
            continue
        addrs.append(int(match.group("addr"), 16))
        symbols.append(match.group("sym"))

    cached = (addrs, symbols)
    _SYMBOL_CACHE[key] = cached
    return cached


def _fallback_symbol(binary: Path, addr: int) -> str | None:
    addrs, symbols = _load_symbol_table(binary)
    if not addrs:
        return None
    idx = bisect_right(addrs, addr) - 1
    if idx < 0:
        return None
    return symbols[idx]


def _is_control_transfer(op: str) -> bool:
    return op.startswith(CONTROL_TRANSFER_PREFIXES)


def _branch_target_addr(operand: object) -> int | None:
    match = TARGET_ADDR_RE.search(str(operand))
    if match is None:
        return None
    return int(match.group("addr"), 16)


def _annotate_output(perf_data: Path, symbol: str) -> str:
    key = (str(perf_data.resolve()), symbol)
    cached = _ANNOTATE_CACHE.get(key)
    if cached is not None:
        return cached
    output = run_cmd(["perf", "annotate", "--stdio", "-i", str(perf_data), "--symbol", symbol]).stdout
    _ANNOTATE_CACHE[key] = output
    return output


def run_addr2line(binary: Path, addrs: list[int]) -> dict[int, tuple[str, str]]:
    if not addrs:
        return {}

    cmd = ["addr2line", "-a", "-i", "-f", "-C", "-e", str(binary)] + [f"0x{addr:x}" for addr in addrs]
    output = run_cmd(cmd).stdout.splitlines()
    mapping: dict[int, tuple[str, str]] = {}

    current_addr: int | None = None
    current_pairs: list[tuple[str, str]] = []

    def flush() -> None:
        nonlocal current_addr, current_pairs
        if current_addr is None:
            return
        fn = "??"
        loc = "??:0"
        for cand_fn, cand_loc in current_pairs:
            if cand_loc and cand_loc != "??:0":
                loc = cand_loc
            if cand_fn and cand_fn != "??":
                fn = cand_fn
                if cand_loc and cand_loc != "??:0":
                    loc = cand_loc
                break
        if fn == "??":
            fallback = _fallback_symbol(binary, current_addr)
            if fallback:
                fn = fallback
        mapping[current_addr] = (fn, loc)
        current_addr = None
        current_pairs = []

    idx = 0
    while idx < len(output):
        line = output[idx].strip()
        if line.startswith("0x"):
            flush()
            try:
                current_addr = int(line, 16)
            except ValueError:
                current_addr = None
            idx += 1
            continue
        if current_addr is not None and idx + 1 < len(output):
            current_pairs.append((line or "??", output[idx + 1].strip() or "??:0"))
            idx += 2
            continue
        idx += 1

    flush()
    for addr in addrs:
        mapping.setdefault(addr, (_fallback_symbol(binary, addr) or "??", "??:0"))
    return mapping


def annotate_basic_blocks(
    perf_data: Path,
    binary: Path,
    event: str,
    symbol_periods: dict[str, float],
    global_total_count: float | None,
    top_n: int,
    max_annotated_symbols: int = DEFAULT_ANNOTATE_SYMBOL_LIMIT,
) -> dict[str, object] | None:
    if not symbol_periods:
        return None

    normalized_event = canonical_event(event)
    annotate_limit = min(max(1, top_n), max(1, max_annotated_symbols))
    hot_symbols = [
        symbol
        for symbol, _ in sorted(symbol_periods.items(), key=lambda kv: kv[1], reverse=True)[:annotate_limit]
    ]
    merged_blocks: dict[tuple[str, int, int], dict[str, object]] = {}
    total_est = 0.0

    for symbol in hot_symbols:
        try:
            ann = _annotate_output(perf_data, symbol)
        except RuntimeError:
            continue

        lines = ann.splitlines()
        group_order: list[str] = []
        for line in lines:
            header = ANN_GROUP_HDR_RE.search(line)
            if header:
                raw_events = [part.strip() for part in header.group("events").split(",") if part.strip()]
                group_order = [canonical_event(item.strip("/ ")) for item in raw_events]
                break

        if group_order and normalized_event in group_order:
            event_idx = group_order.index(normalized_event)
            insns: list[dict[str, object]] = []
            for line in lines:
                insn_match = ANN_GROUP_INSN_RE.match(line)
                if not insn_match:
                    continue
                pcts = [float(token) for token in insn_match.group("pcts").split()]
                if event_idx >= len(pcts):
                    continue
                asm = insn_match.group("asm").strip()
                parts = asm.split(None, 1)
                insns.append(
                    {
                        "addr": int(insn_match.group("addr"), 16),
                        "pct": pcts[event_idx],
                        "op": parts[0] if parts else "",
                        "operand": parts[1] if len(parts) > 1 else "",
                        "asm": asm,
                    }
                )
            if not insns:
                continue

            leaders = {insns[0]["addr"]}
            for idx, insn in enumerate(insns):
                op = str(insn["op"]).lower()
                if _is_control_transfer(op):
                    if idx + 1 < len(insns):
                        leaders.add(insns[idx + 1]["addr"])
                    target_addr = _branch_target_addr(insn["operand"])
                    if target_addr is not None:
                        leaders.add(target_addr)

            blocks: list[list[dict[str, object]]] = []
            current: list[dict[str, object]] = []
            for insn in insns:
                if current and insn["addr"] in leaders:
                    blocks.append(current)
                    current = []
                current.append(insn)
            if current:
                blocks.append(current)

            total_est += symbol_periods.get(symbol, 0.0)
            for block in blocks:
                key = (symbol, int(block[0]["addr"]), int(block[-1]["addr"]))
                entry = merged_blocks.setdefault(
                    key,
                    {
                        "symbol": symbol,
                        "start": block[0]["addr"],
                        "end": block[-1]["addr"],
                        "insn_count": len(block),
                        "event_count_est": 0.0,
                        "preview": [str(ins["asm"]) for ins in block[:4]],
                    },
                )
                entry["event_count_est"] += symbol_periods.get(symbol, 0.0) * sum(float(ins["pct"]) for ins in block) / 100.0
            continue

        sections: list[tuple[int, list[dict[str, object]]]] = []
        section_insns: list[dict[str, object]] = []
        section_samples = 0
        in_target = False

        for line in lines:
            header_match = ANN_EVENT_HDR_RE.search(line)
            if header_match:
                if in_target and section_insns:
                    sections.append((section_samples, section_insns))
                section_insns = []
                pmu_event = canonical_event(header_match.group("pmu").strip())
                section_samples = int(header_match.group("samples"))
                in_target = pmu_event == normalized_event
                continue
            if not in_target:
                continue
            insn_match = ANN_INSN_RE.match(line)
            if not insn_match:
                continue
            asm = insn_match.group("asm").strip()
            parts = asm.split(None, 1)
            section_insns.append(
                {
                    "addr": int(insn_match.group("addr"), 16),
                    "pct": float(insn_match.group("pct")),
                    "op": parts[0] if parts else "",
                    "operand": parts[1] if len(parts) > 1 else "",
                    "asm": asm,
                }
            )
        if in_target and section_insns:
            sections.append((section_samples, section_insns))
        if not sections:
            continue

        symbol_weight = 0.0
        local_blocks: list[dict[str, object]] = []
        for section_samples, insns in sections:
            leaders = {insns[0]["addr"]}
            for idx, insn in enumerate(insns):
                op = str(insn["op"]).lower()
                if _is_control_transfer(op):
                    if idx + 1 < len(insns):
                        leaders.add(insns[idx + 1]["addr"])
                    target_addr = _branch_target_addr(insn["operand"])
                    if target_addr is not None:
                        leaders.add(target_addr)

            blocks: list[list[dict[str, object]]] = []
            current: list[dict[str, object]] = []
            for insn in insns:
                if current and insn["addr"] in leaders:
                    blocks.append(current)
                    current = []
                current.append(insn)
            if current:
                blocks.append(current)

            for block in blocks:
                block_weight = section_samples * sum(float(ins["pct"]) for ins in block) / 100.0
                symbol_weight += block_weight
                local_blocks.append(
                    {
                        "symbol": symbol,
                        "start": block[0]["addr"],
                        "end": block[-1]["addr"],
                        "insn_count": len(block),
                        "preview": [str(ins["asm"]) for ins in block[:4]],
                        "weight": block_weight,
                    }
                )

        if symbol_weight <= 0:
            continue

        scale = symbol_periods.get(symbol, 0.0) / symbol_weight
        total_est += symbol_periods.get(symbol, 0.0)
        for block in local_blocks:
            key = (symbol, int(block["start"]), int(block["end"]))
            est = float(block["weight"]) * scale
            entry = merged_blocks.setdefault(
                key,
                {
                    "symbol": symbol,
                    "start": block["start"],
                    "end": block["end"],
                    "insn_count": block["insn_count"],
                    "event_count_est": 0.0,
                    "preview": block["preview"],
                },
            )
            entry["event_count_est"] += est

    if not merged_blocks:
        return None

    scored = list(merged_blocks.values())
    addr_map: dict[int, tuple[str, str]] = {}
    if binary.exists():
        try:
            addr_map = run_addr2line(binary, [int(row["start"]) for row in scored])
        except RuntimeError:
            addr_map = {}
    for row in scored:
        fn, loc = addr_map.get(int(row["start"]), ("??", "??:0"))
        row["function"] = fn
        row["location"] = loc
        row["coverage_pct"] = 100.0 * float(row["event_count_est"]) / total_est if total_est > 0 else 0.0
        row["global_coverage_pct"] = (
            100.0 * float(row["event_count_est"]) / global_total_count
            if global_total_count is not None and global_total_count > 0
            else None
        )

    scored.sort(key=lambda row: float(row["event_count_est"]), reverse=True)
    block_index = {
        (str(row["symbol"]), int(row["start"]), int(row["end"])): row
        for row in scored
    }
    return {
        "event": event,
        "rows": scored[:top_n],
        "index": block_index,
        "sampled_total_est": total_est,
        "global_total_count": global_total_count,
    }
