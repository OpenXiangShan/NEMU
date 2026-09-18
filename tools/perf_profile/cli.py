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

import argparse
import json
import re
import sys
from pathlib import Path

from .catalog import (
    DEFAULT_BINARY,
    DEFAULT_CALLGRAPH,
    DEFAULT_CALLGRAPH_DEPTH,
    DEFAULT_OUTPUT_ROOT,
    DEFAULT_SAMPLE_FREQ,
    MODES,
    PRECISE_DEFAULT_SAMPLE_EVENTS,
    QUICK_LEADER_EVENT,
    QUICK_MODE,
)
from .collect.record import collect_precise_records, collect_quick_record
from .collect.stat import collect_perf_stat
from .preflight import run_doctor
from .report import generate_reports
from .util import ensure_dir, event_id, make_run_id, progress, read_json, write_json, write_text


RUN_ID_RE = re.compile(r"^[A-Za-z0-9._-]+$")


def _split_nemu_args(text: str) -> list[str]:
    import shlex

    return shlex.split(text) if text else []


def _validate_run_id(run_id: str) -> str:
    if not run_id or run_id in {".", ".."} or RUN_ID_RE.fullmatch(run_id) is None:
        raise ValueError("run-id must match [A-Za-z0-9._-]+")
    return run_id


def _new_run_dir(output_root: Path, run_id: str | None) -> Path:
    if run_id is None:
        run_id = make_run_id("perf")
    else:
        run_id = _validate_run_id(run_id)
    run_dir = output_root / run_id
    if not run_dir.exists():
        return ensure_dir(run_dir)
    suffix = 1
    while True:
        candidate = output_root / f"{run_id}-{suffix}"
        if not candidate.exists():
            return ensure_dir(candidate)
        suffix += 1


def _parse_precise_events(text: str) -> list[str]:
    events: list[str] = []
    seen: set[str] = set()
    for part in text.split(","):
        event = part.strip()
        if not event or event in seen:
            continue
        seen.add(event)
        events.append(event)
    return events


def _validate_precise_events(text: str) -> list[str]:
    events = _parse_precise_events(text)
    for event in events:
        event_id(event)
    return events


def _has_error_warnings(report: dict[str, object]) -> bool:
    warnings = report.get("warnings", [])
    if not isinstance(warnings, list):
        return False
    return any(isinstance(item, dict) and item.get("severity") == "error" for item in warnings)


def _finish_doctor_failure(run_dir: Path, manifest: dict[str, object] | None = None) -> int:
    if manifest is not None:
        write_json(run_dir / "manifest.json", manifest)
    print(run_dir / "reports" / "doctor.md")
    return 1


def _record_stat_failure(stat: dict[str, object]) -> int:
    returncode = int(stat.get("returncode", 0))
    if returncode == 0:
        return 0
    warnings = stat.setdefault("warnings", [])
    if isinstance(warnings, list):
        warnings.append(
            {
                "code": "perf_stat_failed",
                "message": f"perf stat failed with return code {returncode}; record/report results were not generated from this run.",
                "severity": "error",
            }
        )
    return returncode


def _base_manifest(args: argparse.Namespace, run_dir: Path) -> dict[str, object]:
    precise_events = _parse_precise_events(args.precise_events)
    return {
        "run_dir": str(run_dir),
        "config": {
            "mode": args.mode,
            "binary": str(Path(args.binary).resolve()),
            "workload": str(Path(args.workload).resolve()) if getattr(args, "workload", None) else None,
            "nemu_args": _split_nemu_args(args.nemu_args),
            "callgraph": args.callgraph,
            "callgraph_depth": args.callgraph_depth,
            "include_external": args.include_external,
            "sample_freq": args.sample_freq,
            "top": args.top,
            "bb_top": args.bb_top,
        },
        "sampling": {
            "quick_leader_event": QUICK_LEADER_EVENT,
            "sampled_events": precise_events if args.mode != QUICK_MODE else [],
            "record_artifacts": {},
        },
        "warnings": [],
        "reports": {},
    }


def _write_doctor(run_dir: Path, doctor: dict[str, object]) -> None:
    write_json(run_dir / "parsed" / "doctor.json", doctor)
    lines = ["# Doctor", ""]
    for key, value in doctor["checks"].items():
        lines.append(f"- {key}: {value}")
    if doctor["warnings"]:
        lines.append("")
        lines.append("## Warnings")
        for warning in doctor["warnings"]:
            lines.append(f"- [{warning['severity']}] {warning['code']}: {warning['message']}")
    write_text(run_dir / "reports" / "doctor.md", "\n".join(lines) + "\n")


def _copy_nemu_stdout(run_dir: Path, stat: dict[str, object]) -> None:
    stdout_log = stat.get("stdout_log")
    if not stdout_log:
        return
    stdout_path = Path(str(stdout_log))
    if not stdout_path.is_absolute():
        stdout_path = run_dir / stdout_path
    if stdout_path.exists():
        write_text(
            run_dir / "logs" / "nemu.stdout.log",
            stdout_path.read_text(encoding="utf-8", errors="replace"),
        )


def command_doctor(args: argparse.Namespace) -> int:
    run_dir = _new_run_dir(Path(args.output_root), args.run_id)
    report = run_doctor(Path(args.binary), Path(args.workload).resolve() if args.workload else None)
    _write_doctor(run_dir, report)
    print(run_dir / "reports" / "doctor.md")
    return 1 if _has_error_warnings(report) else 0


def _record(run_dir: Path, manifest: dict[str, object]) -> None:
    config = manifest["config"]
    binary = Path(config["binary"])
    workload = Path(config["workload"])
    nemu_args = list(config["nemu_args"])
    raw_record_dir = ensure_dir(run_dir / "raw" / "record")
    if config["mode"] == QUICK_MODE:
        artifact, samples = collect_quick_record(
            binary=binary,
            workload=workload,
            nemu_args=nemu_args,
            out_dir=raw_record_dir,
            freq=int(config["sample_freq"]),
            callgraph=str(config["callgraph"]),
            callgraph_depth=int(config["callgraph_depth"]),
            include_external=bool(config["include_external"]),
        )
        manifest["sampling"]["sampled_events"] = list(artifact["sampled_events"])
        manifest["sampling"]["record_artifacts"]["quick"] = artifact
        write_json(run_dir / "parsed" / "quick_samples.json", samples)
    else:
        events = list(manifest["sampling"]["sampled_events"])
        artifacts, samples_by_event = collect_precise_records(
            binary=binary,
            workload=workload,
            nemu_args=nemu_args,
            out_dir=raw_record_dir,
            events=events,
            freq=int(config["sample_freq"]),
            callgraph=str(config["callgraph"]),
            callgraph_depth=int(config["callgraph_depth"]),
            include_external=bool(config["include_external"]),
        )
        manifest["sampling"]["record_artifacts"] = artifacts
        for event, samples in samples_by_event.items():
            write_json(run_dir / "parsed" / f"samples_{event_id(event)}.json", samples)


def command_stat(args: argparse.Namespace) -> int:
    run_dir = _new_run_dir(Path(args.output_root), args.run_id)
    manifest = _base_manifest(args, run_dir)
    progress(1, 2, "doctor")
    doctor = run_doctor(Path(manifest["config"]["binary"]), Path(manifest["config"]["workload"]))
    manifest["warnings"].extend(doctor["warnings"])
    _write_doctor(run_dir, doctor)
    if _has_error_warnings(doctor):
        return _finish_doctor_failure(run_dir, manifest)
    progress(2, 2, "stat")
    stat = collect_perf_stat(
        binary=Path(manifest["config"]["binary"]),
        workload=Path(manifest["config"]["workload"]),
        nemu_args=list(manifest["config"]["nemu_args"]),
        out_dir=ensure_dir(run_dir / "raw" / "stat"),
    )
    _copy_nemu_stdout(run_dir, stat)
    stat_returncode = _record_stat_failure(stat)
    write_json(run_dir / "parsed" / "stat.json", stat)
    write_json(run_dir / "manifest.json", manifest)
    print(run_dir)
    return stat_returncode


def command_record(args: argparse.Namespace) -> int:
    run_dir = _new_run_dir(Path(args.output_root), args.run_id)
    manifest = _base_manifest(args, run_dir)
    progress(1, 2, "doctor")
    doctor = run_doctor(Path(manifest["config"]["binary"]), Path(manifest["config"]["workload"]))
    manifest["warnings"].extend(doctor["warnings"])
    _write_doctor(run_dir, doctor)
    if _has_error_warnings(doctor):
        return _finish_doctor_failure(run_dir, manifest)
    progress(2, 2, "record", manifest["config"]["mode"])
    _record(run_dir, manifest)
    write_json(run_dir / "manifest.json", manifest)
    print(run_dir)
    return 0


def command_report(args: argparse.Namespace) -> int:
    run_dir = Path(args.from_run).resolve()
    manifest_path = run_dir / "manifest.json"
    try:
        manifest = read_json(manifest_path)
    except (FileNotFoundError, OSError, json.JSONDecodeError) as exc:
        print(f"error: failed to read {manifest_path}: {exc}", file=sys.stderr)
        return 1
    if not isinstance(manifest, dict):
        print(f"error: {manifest_path} must contain a JSON object", file=sys.stderr)
        return 1
    reports, warnings = generate_reports(run_dir, manifest, top_n=args.top, bb_top_n=args.bb_top)
    manifest["reports"] = reports
    manifest["warnings"] = warnings
    write_json(run_dir / "manifest.json", manifest)
    print(run_dir / reports["summary"])
    return 0


def command_run(args: argparse.Namespace) -> int:
    run_dir = _new_run_dir(Path(args.output_root), args.run_id)
    manifest = _base_manifest(args, run_dir)
    total_steps = 4
    progress(1, total_steps, "doctor")
    doctor = run_doctor(Path(manifest["config"]["binary"]), Path(manifest["config"]["workload"]))
    manifest["warnings"].extend(doctor["warnings"])
    _write_doctor(run_dir, doctor)
    if _has_error_warnings(doctor):
        return _finish_doctor_failure(run_dir, manifest)

    progress(2, total_steps, "stat")
    stat = collect_perf_stat(
        binary=Path(manifest["config"]["binary"]),
        workload=Path(manifest["config"]["workload"]),
        nemu_args=list(manifest["config"]["nemu_args"]),
        out_dir=ensure_dir(run_dir / "raw" / "stat"),
    )
    _copy_nemu_stdout(run_dir, stat)
    stat_returncode = _record_stat_failure(stat)
    write_json(run_dir / "parsed" / "stat.json", stat)
    if stat_returncode != 0:
        write_json(run_dir / "manifest.json", manifest)
        print(run_dir)
        return stat_returncode

    progress(3, total_steps, "record", manifest["config"]["mode"])
    _record(run_dir, manifest)

    progress(4, total_steps, "report")
    reports, warnings = generate_reports(
        run_dir,
        manifest,
        top_n=int(manifest["config"]["top"]),
        bb_top_n=int(manifest["config"]["bb_top"]),
    )
    manifest["reports"] = reports
    manifest["warnings"] = warnings
    write_json(run_dir / "manifest.json", manifest)
    print(run_dir / reports["summary"])
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="NEMU perf profiling tool")
    subparsers = parser.add_subparsers(dest="command", required=True)

    def add_common(sub: argparse.ArgumentParser, require_workload: bool = True) -> None:
        sub.add_argument("--binary", default=str(DEFAULT_BINARY))
        sub.add_argument("--workload", required=require_workload)
        sub.add_argument("--output-root", default=str(DEFAULT_OUTPUT_ROOT))
        sub.add_argument("--run-id")
        sub.add_argument("--mode", choices=MODES, default=QUICK_MODE)
        sub.add_argument(
            "--nemu-args",
            default="-b",
            help="arguments passed to NEMU before the workload; for Linux workloads, a practical starting point is '-b -I 1000000000'",
        )
        sub.add_argument("--callgraph", choices=["fp", "dwarf"], default=DEFAULT_CALLGRAPH)
        sub.add_argument(
            "--callgraph-depth",
            type=int,
            default=DEFAULT_CALLGRAPH_DEPTH,
            help="stack dump size used by perf only when --callgraph=dwarf; ignored for --callgraph=fp",
        )
        sub.add_argument("--sample-freq", type=int, default=DEFAULT_SAMPLE_FREQ)
        sub.add_argument("--top", type=int, default=100)
        sub.add_argument("--bb-top", type=int, default=100)
        sub.add_argument("--include-external", action="store_true")
        sub.add_argument("--precise-events", default=",".join(PRECISE_DEFAULT_SAMPLE_EVENTS))

    doctor = subparsers.add_parser("doctor", help="run environment checks")
    doctor.add_argument("--binary", default=str(DEFAULT_BINARY))
    doctor.add_argument("--workload")
    doctor.add_argument("--output-root", default=str(DEFAULT_OUTPUT_ROOT))
    doctor.add_argument("--run-id")
    doctor.set_defaults(func=command_doctor)

    stat = subparsers.add_parser("stat", help="collect perf stat counters")
    add_common(stat)
    stat.set_defaults(func=command_stat)

    record = subparsers.add_parser("record", help="collect perf record samples")
    add_common(record)
    record.set_defaults(func=command_record)

    report = subparsers.add_parser("report", help="regenerate reports from an existing run")
    report.add_argument("--from-run", required=True)
    report.add_argument("--top", type=int, default=100)
    report.add_argument("--bb-top", type=int, default=100)
    report.set_defaults(func=command_report)

    run = subparsers.add_parser("run", help="run doctor, stat, record, and report")
    add_common(run)
    run.set_defaults(func=command_run)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if getattr(args, "run_id", None) is not None:
        try:
            _validate_run_id(args.run_id)
        except ValueError as exc:
            parser.error(f"--run-id {exc}")
    if getattr(args, "command", "") in {"record", "run"}:
        if int(args.sample_freq) <= 0:
            parser.error("--sample-freq must be positive")
        if int(args.callgraph_depth) <= 0:
            parser.error("--callgraph-depth must be positive")
    if getattr(args, "command", "") in {"run", "report"}:
        if int(args.top) <= 0:
            parser.error("--top must be positive")
        if int(args.bb_top) <= 0:
            parser.error("--bb-top must be positive")
    if getattr(args, "mode", QUICK_MODE) != QUICK_MODE and getattr(args, "command", "") in {"stat", "record", "run"}:
        try:
            precise_events = _validate_precise_events(args.precise_events)
        except ValueError as exc:
            parser.error(f"--precise-events contains an invalid event: {exc}")
        if not precise_events:
            parser.error("--precise-events must specify at least one event in precise mode")
    return args.func(args)
