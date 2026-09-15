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

import json
from collections.abc import Iterator
from collections import defaultdict
from pathlib import Path

from .catalog import GLOBAL_EVENTS, QUICK_GROUP_EVENTS, QUICK_LEADER_EVENT, QUICK_MODE
from .parse import annotate_basic_blocks
from .util import canonical_event, derive_metrics, event_id, fmt_count, fmt_num, parse_nemu_summary, write_json, write_text


def _iter_sample_records(records: list[object]) -> Iterator[tuple[dict[str, object], str, str, float]]:
    for record in records:
        if not isinstance(record, dict):
            continue
        event = record.get("event")
        leaf = record.get("leaf")
        period = record.get("period")
        if event is None or leaf is None or period is None:
            continue
        try:
            yield record, str(event), str(leaf), float(period)
        except (TypeError, ValueError):
            continue


def _aggregate_precise_functions(records: list[dict[str, object]], event: str) -> list[dict[str, object]]:
    normalized_event = canonical_event(event)
    agg: dict[str, dict[str, object]] = defaultdict(lambda: {"function": "", "counters": {event: 0.0}, "samples": {event: 0}})
    for _, sample_event, leaf, period in _iter_sample_records(records):
        if canonical_event(sample_event) != normalized_event:
            continue
        entry = agg[leaf]
        entry["function"] = leaf
        entry["counters"][event] = entry["counters"].get(event, 0.0) + period
        entry["samples"][event] = entry["samples"].get(event, 0) + 1
    rows = list(agg.values())
    rows.sort(key=lambda row: float(row["counters"].get(event, 0.0)), reverse=True)
    return rows


def aggregate_quick_functions(records: list[dict[str, object]]) -> list[dict[str, object]]:
    by_group: dict[str, dict[str, dict[str, object]]] = defaultdict(dict)
    for record, event, leaf, period in _iter_sample_records(records):
        group_id = record.get("group_id")
        if group_id is None:
            continue
        by_group[str(group_id)][event] = {"leaf": leaf, "period": period}

    agg: dict[str, dict[str, object]] = defaultdict(
        lambda: {"function": "", "counters": defaultdict(float), "samples": defaultdict(int)}
    )
    for group in by_group.values():
        leader = group.get(QUICK_LEADER_EVENT)
        if not leader:
            continue
        fn = str(leader["leaf"])
        entry = agg[fn]
        entry["function"] = fn
        for event, sample in group.items():
            entry["counters"][event] += float(sample["period"])
            entry["samples"][event] += 1

    rows = list(agg.values())
    rows.sort(key=lambda row: float(row["counters"].get(QUICK_LEADER_EVENT, 0.0)), reverse=True)
    for row in rows:
        row["counters"] = dict(row["counters"])
        row["samples"] = dict(row["samples"])
        row["derived"] = derive_metrics(row["counters"])
    return rows


def _render_warning_section(lines: list[str], warnings: list[dict[str, object]]) -> None:
    if not warnings:
        return
    lines.append("")
    lines.append("## Warnings")
    for warning in warnings:
        lines.append(f"- [{warning.get('severity', 'warning')}] {warning.get('code')}: {warning.get('message')}")


def summarize_sample_counts(parsed_dir: Path, manifest: dict[str, object]) -> dict[str, object]:
    mode = str(manifest["config"]["mode"])
    sampled_events = [str(event) for event in manifest["sampling"].get("sampled_events", [])]

    if mode == QUICK_MODE:
        quick_samples_path = parsed_dir / "quick_samples.json"
        samples = _read_json_list(quick_samples_path)
        if samples is None:
            return {"mode": mode, "group_count": 0, "event_counts": {}}
        event_counts: dict[str, int] = defaultdict(int)
        group_ids: set[str] = set()
        for sample in samples:
            if not isinstance(sample, dict):
                continue
            event = str(sample.get("event", ""))
            group_id = str(sample.get("group_id", ""))
            if not event:
                continue
            event_counts[event] += 1
            if group_id:
                group_ids.add(group_id)
        return {
            "mode": mode,
            "group_count": len(group_ids),
            "event_counts": dict(event_counts),
        }

    event_counts: dict[str, int] = {}
    for event in sampled_events:
        sample_path = parsed_dir / f"samples_{event_id(event)}.json"
        samples = _read_json_list(sample_path)
        if samples is None:
            continue
        event_counts[event] = sum(1 for sample in samples if isinstance(sample, dict))
    return {"mode": mode, "event_counts": event_counts}


def _read_json_list(path: Path) -> list[object] | None:
    if not path.exists():
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError):
        return None
    if not isinstance(data, list):
        return None
    return data


def _read_json_dict(path: Path) -> dict[str, object] | None:
    if not path.exists():
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError):
        return None
    if not isinstance(data, dict):
        return None
    return data


def write_precise_function_report(path: Path, event: str, rows: list[dict[str, object]], global_total: float | None, top_n: int) -> None:
    top_rows = rows[:top_n]
    sampled_total = sum(float(row["counters"].get(event, 0.0)) for row in rows)
    lines = [
        f"# Functions for {event}",
        "",
        "- mode: precise",
        "- attribution: event-triggered sampled self period",
        f"- sampled total {event}: {sampled_total:.0f}",
    ]
    if global_total is None:
        lines.append(f"- global total {event}: unavailable")
        lines.append("- note: precise mode samples each event in a separate NEMU run, so summary-wide global totals are not reused here")
    else:
        lines.append(f"- global total {event}: {global_total:.0f}")
    lines.extend([
        "",
        "| rank | function | event_count_est | pct_of_sampled_event | samples |",
        "|---|---|---:|---:|---:|",
    ])
    for idx, row in enumerate(top_rows, 1):
        count = float(row["counters"].get(event, 0.0))
        pct = 100.0 * count / sampled_total if sampled_total > 0 else 0.0
        lines.append(f"| {idx} | {row['function']} | {count:.0f} | {pct:.2f}% | {row['samples'].get(event, 0)} |")
    write_text(path, "\n".join(lines) + "\n")


def write_quick_function_report(path: Path, rows: list[dict[str, object]], top_n: int) -> None:
    top_rows = rows[:top_n]
    leader_total = sum(float(row["counters"].get(QUICK_LEADER_EVENT, 0.0)) for row in rows)
    lines = [
        "# Quick Mode Function Hotspots",
        "",
        "- mode: quick",
        f"- leader event: {QUICK_LEADER_EVENT}",
        "- note: only instructions is the trigger event; other counters are grouped readouts from the same samples",
        "",
        "| rank | function | instructions | cycles | branches | branch-misses | cache-references | cache-misses | cpi | bp_mpki | cache_mpki |",
        "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for idx, row in enumerate(top_rows, 1):
        ctrs = row["counters"]
        derived = row["derived"]
        lines.append(
            "| {idx} | {fn} | {ins} | {cyc} | {br} | {bm} | {cr} | {cm} | {cpi} | {bp} | {cache} |".format(
                idx=idx,
                fn=row["function"],
                ins=fmt_count(ctrs.get("instructions")),
                cyc=fmt_count(ctrs.get("cycles")),
                br=fmt_count(ctrs.get("branches")),
                bm=fmt_count(ctrs.get("branch-misses")),
                cr=fmt_count(ctrs.get("cache-references")),
                cm=fmt_count(ctrs.get("cache-misses")),
                cpi=fmt_num(derived["cpi"]),
                bp=fmt_num(derived["bp_mpki"]),
                cache=fmt_num(derived["cache_mpki"]),
            )
        )
    lines.append("")
    lines.append(f"- sampled total instructions: {leader_total:.0f}")
    write_text(path, "\n".join(lines) + "\n")


def write_precise_block_report(path: Path, result: dict[str, object], top_n: int) -> None:
    rows = list(result["rows"])[:top_n]
    event = str(result["event"])
    global_total = result["global_total_count"]
    lines = [
        f"# Basic Blocks for {event}",
        "",
        "- mode: precise",
        "- attribution: event-triggered sampled attribution projected to basic blocks via perf annotate",
        f"- sampled total {event}: {float(result['sampled_total_est']):.0f}",
    ]
    if global_total is None:
        lines.append(f"- global total {event}: unavailable")
        lines.append("- note: precise mode samples each event in a separate NEMU run, so global coverage is omitted")
    else:
        lines.append(f"- global total {event}: {float(global_total):.0f}")
    lines.append("")
    if global_total is None:
        lines.extend([
            "| rank | block_start | block_end | event_count_est | sampled_coverage | insns | function | source |",
            "|---|---:|---:|---:|---:|---:|---|---|",
        ])
    else:
        lines.extend([
            "| rank | block_start | block_end | event_count_est | sampled_coverage | global_coverage | insns | function | source |",
            "|---|---:|---:|---:|---:|---:|---:|---|---|",
        ])
    for idx, row in enumerate(rows, 1):
        if global_total is None:
            lines.append(
                f"| {idx} | 0x{int(row['start']):x} | 0x{int(row['end']):x} | {float(row['event_count_est']):.0f} | {float(row['coverage_pct']):.2f}% | {row['insn_count']} | {row['function']} | {row['location']} |"
            )
        else:
            lines.append(
                f"| {idx} | 0x{int(row['start']):x} | 0x{int(row['end']):x} | {float(row['event_count_est']):.0f} | {float(row['coverage_pct']):.2f}% | {float(row['global_coverage_pct']):.2f}% | {row['insn_count']} | {row['function']} | {row['location']} |"
            )
    write_text(path, "\n".join(lines) + "\n")


def merge_quick_blocks(per_event_results: dict[str, dict[str, object]], top_n: int) -> list[dict[str, object]]:
    leader_rows = list(per_event_results[QUICK_LEADER_EVENT]["rows"])[:top_n]
    indices = {event: result["index"] for event, result in per_event_results.items()}
    merged: list[dict[str, object]] = []
    for row in leader_rows:
        key = (str(row["symbol"]), int(row["start"]), int(row["end"]))
        counters = {}
        for event, index in indices.items():
            counters[event] = float(index.get(key, {}).get("event_count_est", 0.0))
        merged_row = dict(row)
        merged_row["counters"] = counters
        merged_row["derived"] = derive_metrics(counters)
        merged.append(merged_row)
    return merged


def _serialize_block_result(result: dict[str, object]) -> dict[str, object]:
    return {
        key: value
        for key, value in result.items()
        if key != "index"
    }


def write_quick_block_report(path: Path, rows: list[dict[str, object]]) -> None:
    lines = [
        "# Quick Mode Basic Block Hotspots",
        "",
        "- mode: quick",
        f"- leader event: {QUICK_LEADER_EVENT}",
        "- note: ranked by instruction-triggered samples; other counters are grouped readouts projected onto the same blocks",
        "",
        "| rank | block_start | block_end | instructions | cycles | branch-misses | cache-misses | cpi | bp_mpki | cache_mpki | function | source |",
        "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|",
    ]
    for idx, row in enumerate(rows, 1):
        ctrs = row["counters"]
        derived = row["derived"]
        lines.append(
            f"| {idx} | 0x{int(row['start']):x} | 0x{int(row['end']):x} | {fmt_count(ctrs.get('instructions'))} | {fmt_count(ctrs.get('cycles'))} | {fmt_count(ctrs.get('branch-misses'))} | {fmt_count(ctrs.get('cache-misses'))} | {fmt_num(derived['cpi'])} | {fmt_num(derived['bp_mpki'])} | {fmt_num(derived['cache_mpki'])} | {row['function']} | {row['location']} |"
        )
    write_text(path, "\n".join(lines) + "\n")


def write_summary_report(
    path: Path,
    manifest: dict[str, object],
    stat: dict[str, object] | None,
    sample_summary: dict[str, object],
    warnings: list[dict[str, object]],
    reports: dict[str, str],
    logs_dir: Path,
) -> None:
    config = manifest["config"]
    stdout_path = logs_dir / "nemu.stdout.log"
    stdout_text = stdout_path.read_text(encoding="utf-8", errors="replace") if stdout_path.exists() else ""
    guest_instr, sim_freq = parse_nemu_summary(stdout_text)

    lines = [
        "# NEMU perf profile summary",
        "",
        "## Goal",
        "- quantify end-to-end host-side counter cost",
        "- localize hot functions and basic blocks",
        "- support quick overview and precise diagnosis modes",
        "",
        "## Run Summary",
        f"- mode: {config['mode']}",
        f"- binary: {config['binary']}",
        f"- workload: {config.get('workload')}",
        f"- quick leader event: {manifest['sampling'].get('quick_leader_event')}",
        f"- sampled events: {', '.join(manifest['sampling'].get('sampled_events', []))}",
    ]
    if guest_instr is not None:
        lines.append(f"- guest instructions: {guest_instr}")
    if sim_freq is not None:
        lines.append(f"- simulation frequency: {sim_freq} instr/s")

    if stat:
        lines.extend(["", "## Global Counters", "| event | count | running_pct |", "|---|---:|---:|"])
        counts = stat.get("counts", {})
        running_pct = stat.get("running_pct", {})
        for event in GLOBAL_EVENTS:
            lines.append(
                f"| {event} | {fmt_count(counts.get(event))} | {fmt_num(running_pct.get(event))} |"
            )

    event_counts = sample_summary.get("event_counts", {})
    if event_counts:
        lines.extend(["", "## Sample Counts"])
        if sample_summary.get("mode") == QUICK_MODE:
            lines.append(f"- grouped sample groups: {sample_summary.get('group_count', 0)}")
        lines.extend(["| event | samples |", "|---|---:|"])
        for event in manifest["sampling"].get("sampled_events", []):
            lines.append(f"| {event} | {event_counts.get(event, 0)} |")

    lines.extend(["", "## Reports"])
    for name, rel_path in reports.items():
        lines.append(f"- {name}: {rel_path}")

    _render_warning_section(lines, warnings)
    write_text(path, "\n".join(lines) + "\n")


def generate_reports(run_dir: Path, manifest: dict[str, object], top_n: int = 100, bb_top_n: int = 100) -> tuple[dict[str, str], list[dict[str, object]]]:
    parsed_dir = run_dir / "parsed"
    reports_dir = run_dir / "reports"
    reports_dir.mkdir(parents=True, exist_ok=True)
    config = manifest["config"]
    binary = Path(config["binary"])
    mode = str(config["mode"])
    stat = None
    warnings: list[dict[str, object]] = []
    doctor_path = parsed_dir / "doctor.json"
    doctor = _read_json_dict(doctor_path)
    if doctor is None:
        if doctor_path.exists():
            warnings.append(
                {
                    "code": "invalid_doctor_json",
                    "message": "doctor.json is invalid; doctor warnings could not be loaded.",
                    "severity": "warning",
                }
            )
    else:
        doctor_warnings = doctor.get("warnings", [])
        if isinstance(doctor_warnings, list):
            warnings.extend(doctor_warnings)
    stat_path = parsed_dir / "stat.json"
    stat = _read_json_dict(stat_path)
    if stat is None:
        if stat_path.exists():
            warnings.append(
                {
                    "code": "invalid_stat_json",
                    "message": "stat.json is invalid; global counters could not be loaded.",
                    "severity": "warning",
                }
            )
    else:
        stat_warnings = stat.get("warnings", [])
        if isinstance(stat_warnings, list):
            warnings.extend(stat_warnings)

    reports: dict[str, str] = {}
    raw_records = manifest["sampling"].get("record_artifacts", {})
    sample_summary = summarize_sample_counts(parsed_dir, manifest)

    doctor_report_path = reports_dir / "doctor.md"
    if doctor_report_path.exists():
        reports["doctor"] = str(doctor_report_path.relative_to(run_dir))

    if mode == QUICK_MODE:
        warnings.append(
            {
                "code": "quick_mode_semantics",
                "message": "quick mode ranks hotspots by instruction-triggered samples; grouped counters are auxiliary readouts, not standalone event-triggered hotspots.",
                "severity": "info",
            }
        )
        quick_artifact = raw_records.get("quick")
        quick_samples_path = parsed_dir / "quick_samples.json"
        samples = _read_json_list(quick_samples_path)
        if samples is None:
            warnings.append(
                {
                    "code": "missing_quick_samples",
                    "message": "quick-mode parsed samples are missing or invalid; skipping quick sample reports.",
                    "severity": "warning",
                }
            )
            samples = []

        if samples:
            function_rows = aggregate_quick_functions(samples)
            quick_func_path = reports_dir / "functions_quick.md"
            write_quick_function_report(quick_func_path, function_rows, top_n)
            reports["functions_quick"] = str(quick_func_path.relative_to(run_dir))
            write_json(parsed_dir / "quick_functions.json", function_rows)

        if quick_artifact is None:
            warnings.append(
                {
                    "code": "missing_quick_artifact",
                    "message": "quick-mode record artifacts are missing; skipping quick basic-block reports.",
                    "severity": "warning",
                }
            )
        elif "perf_data" not in quick_artifact:
            warnings.append(
                {
                    "code": "invalid_quick_artifact",
                    "message": "quick-mode record artifacts are incomplete; skipping quick basic-block reports.",
                    "severity": "warning",
                }
            )

        per_event_results: dict[str, dict[str, object]] = {}
        if samples and quick_artifact is not None and "perf_data" in quick_artifact:
            for event in QUICK_GROUP_EVENTS:
                event_symbol_periods: dict[str, float] = defaultdict(float)
                event_sample_count = 0
                for _, sample_event, leaf, period in _iter_sample_records(samples):
                    if sample_event != event:
                        continue
                    event_symbol_periods[leaf] += period
                    event_sample_count += 1
                if event_sample_count < 20:
                    warnings.append(
                        {
                            "code": "low_sample_count",
                            "message": f"{event} collected fewer than 20 grouped samples; quick-mode auxiliary metrics may be noisy.",
                            "severity": "warning",
                        }
                    )
                result = annotate_basic_blocks(
                    perf_data=run_dir / str(quick_artifact["perf_data"]),
                    binary=binary,
                    event=event,
                    symbol_periods=dict(event_symbol_periods),
                    global_total_count=None,
                    top_n=bb_top_n,
                )
                if result is None:
                    warnings.append(
                        {
                            "code": "annotate_failed",
                            "message": f"perf annotate did not yield blocks for {event} in quick mode.",
                            "severity": "warning",
                        }
                    )
                    continue
                per_event_results[event] = result
        if QUICK_LEADER_EVENT in per_event_results:
            quick_block_rows = merge_quick_blocks(per_event_results, bb_top_n)
            quick_bb_path = reports_dir / "basic_blocks_quick.md"
            write_quick_block_report(quick_bb_path, quick_block_rows)
            reports["basic_blocks_quick"] = str(quick_bb_path.relative_to(run_dir))
            write_json(parsed_dir / "basic_blocks_quick.json", quick_block_rows)
    else:
        for event in manifest["sampling"].get("sampled_events", []):
            sample_path = parsed_dir / f"samples_{event_id(event)}.json"
            samples = _read_json_list(sample_path)
            if samples is None:
                warnings.append(
                    {
                        "code": "missing_precise_samples",
                        "message": f"{event} parsed samples are missing or invalid; skipping precise reports for this event.",
                        "severity": "warning",
                    }
                )
                continue

            valid_samples = 0
            normalized_event = canonical_event(event)
            event_symbol_periods: dict[str, float] = defaultdict(float)
            for _, sample_event, leaf, period in _iter_sample_records(samples):
                if canonical_event(sample_event) != normalized_event:
                    continue
                event_symbol_periods[leaf] += period
                valid_samples += 1
            if valid_samples < 20:
                warnings.append(
                    {
                        "code": "low_sample_count",
                        "message": f"{event} collected fewer than 20 samples; event-triggered attribution may be low confidence.",
                        "severity": "warning",
                    }
                )

            function_rows = _aggregate_precise_functions(samples, event)
            event_slug = event_id(event)
            func_path = reports_dir / f"functions_{event_slug}.md"
            write_precise_function_report(func_path, event, function_rows, None, top_n)
            reports[f"functions_{event_slug}"] = str(func_path.relative_to(run_dir))
            write_json(parsed_dir / f"functions_{event_slug}.json", function_rows)

            artifact = raw_records.get(event)
            if artifact is None or "perf_data" not in artifact:
                warnings.append(
                    {
                        "code": "missing_precise_artifact",
                        "message": f"{event} record artifacts are missing; skipping precise basic-block reports for this event.",
                        "severity": "warning",
                    }
                )
                continue

            block_result = annotate_basic_blocks(
                perf_data=run_dir / str(artifact["perf_data"]),
                binary=binary,
                event=event,
                symbol_periods=dict(event_symbol_periods),
                global_total_count=None,
                top_n=bb_top_n,
            )
            if block_result is None:
                warnings.append(
                    {
                        "code": "annotate_failed",
                        "message": f"perf annotate did not yield blocks for {event}.",
                        "severity": "warning",
                    }
                )
                continue
            bb_path = reports_dir / f"basic_blocks_{event_slug}.md"
            write_precise_block_report(bb_path, block_result, bb_top_n)
            reports[f"basic_blocks_{event_slug}"] = str(bb_path.relative_to(run_dir))
            write_json(parsed_dir / f"basic_blocks_{event_slug}.json", _serialize_block_result(block_result))

    summary_path = reports_dir / "summary.md"
    write_summary_report(summary_path, manifest, stat, sample_summary, warnings, reports, run_dir / "logs")
    reports["summary"] = str(summary_path.relative_to(run_dir))
    return reports, warnings
