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

from pathlib import Path

from ..catalog import QUICK_GROUP_EVENTS, QUICK_LEADER_EVENT
from ..parse import parse_script_samples
from ..util import event_id, run_cmd


SCRIPT_FIELDS = "comm,pid,tid,time,event,ip,sym,dso,period"


def _callgraph_arg(mode: str, depth: int) -> str:
    normalized_mode = mode.strip().lower()
    if normalized_mode == "dwarf":
        return f"{normalized_mode},{depth}"
    return normalized_mode


def _record_one(
    *,
    binary: Path,
    workload: Path,
    nemu_args: list[str],
    out_dir: Path,
    run_dir: Path,
    name: str,
    event_expr: str,
    freq: int,
    callgraph: str,
    callgraph_depth: int,
    include_external: bool,
) -> tuple[dict[str, object], list[dict[str, object]]]:
    out_dir.mkdir(parents=True, exist_ok=True)
    perf_data = out_dir / f"{name}.data"
    script_txt = out_dir / f"{name}.script.txt"
    stderr_path = out_dir / f"{name}.record.stderr.log"
    stdout_path = out_dir / f"{name}.record.stdout.log"
    cmd = [
        "perf", "record", "--all-user",
        "--call-graph", _callgraph_arg(callgraph, callgraph_depth),
        "-F", str(freq), "-e", event_expr, "-o", str(perf_data),
        "--", str(binary), *nemu_args, str(workload),
    ]
    proc = run_cmd(cmd, check=False, stdout_path=stdout_path, stderr_path=stderr_path)
    if proc.returncode != 0:
        raise RuntimeError(f"perf record failed for {name}, see {stderr_path}")

    run_cmd(["perf", "script", "-i", str(perf_data), "-F", SCRIPT_FIELDS], stdout_path=script_txt)
    with script_txt.open("r", encoding="utf-8") as script_fp:
        samples = parse_script_samples(script_fp, binary.name, include_external=include_external)
    artifact = {
        "freq": freq,
        "perf_data": str(perf_data.relative_to(run_dir)),
        "script_txt": str(script_txt.relative_to(run_dir)),
        "stderr_log": str(stderr_path.relative_to(run_dir)),
        "stdout_log": str(stdout_path.relative_to(run_dir)),
        "command": cmd,
    }
    return artifact, [sample.__dict__ for sample in samples]


def collect_quick_record(
    binary: Path,
    workload: Path,
    nemu_args: list[str],
    out_dir: Path,
    freq: int,
    callgraph: str,
    callgraph_depth: int,
    include_external: bool,
) -> tuple[dict[str, object], list[dict[str, object]]]:
    event_expr = "{" + ",".join(QUICK_GROUP_EVENTS) + "}:S"
    run_dir = out_dir.parent.parent
    artifact, samples = _record_one(
        binary=binary,
        workload=workload,
        nemu_args=nemu_args,
        out_dir=out_dir,
        run_dir=run_dir,
        name="quick",
        event_expr=event_expr,
        freq=freq,
        callgraph=callgraph,
        callgraph_depth=callgraph_depth,
        include_external=include_external,
    )
    artifact.update({"mode": "quick", "leader_event": QUICK_LEADER_EVENT, "sampled_events": QUICK_GROUP_EVENTS})
    return artifact, samples


def collect_precise_records(
    binary: Path,
    workload: Path,
    nemu_args: list[str],
    out_dir: Path,
    events: list[str],
    freq: int,
    callgraph: str,
    callgraph_depth: int,
    include_external: bool,
) -> tuple[dict[str, dict[str, object]], dict[str, list[dict[str, object]]]]:
    artifacts: dict[str, dict[str, object]] = {}
    samples_by_event: dict[str, list[dict[str, object]]] = {}
    run_dir = out_dir.parent.parent
    for event in events:
        event_slug = event_id(event)
        event_dir = out_dir / event_slug
        artifact, samples = _record_one(
            binary=binary,
            workload=workload,
            nemu_args=nemu_args,
            out_dir=event_dir,
            run_dir=run_dir,
            name=event_slug,
            event_expr=event,
            freq=freq,
            callgraph=callgraph,
            callgraph_depth=callgraph_depth,
            include_external=include_external,
        )
        artifact.update({"mode": "precise", "event": event})
        artifacts[event] = artifact
        samples_by_event[event] = samples
    return artifacts, samples_by_event
