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

from pathlib import Path


DEFAULT_BINARY = Path("build/riscv64-nemu-interpreter")
DEFAULT_OUTPUT_ROOT = Path("perf_profile_out")

GLOBAL_EVENTS = [
    "cycles",
    "instructions",
    "branches",
    "branch-misses",
    "cache-references",
    "cache-misses",
]

QUICK_MODE = "quick"
PRECISE_MODE = "precise"
MODES = (QUICK_MODE, PRECISE_MODE)

QUICK_LEADER_EVENT = "instructions"
QUICK_GROUP_EVENTS = [QUICK_LEADER_EVENT] + [
    event for event in GLOBAL_EVENTS if event != QUICK_LEADER_EVENT
]
PRECISE_DEFAULT_SAMPLE_EVENTS = ["instructions", "cycles", "branch-misses", "cache-misses"]

DERIVED_METRICS = ["ipc", "cpi", "bp_mpki", "cache_mpki", "branch_miss_rate"]

DEFAULT_SAMPLE_FREQ = 999
DEFAULT_CALLGRAPH = "fp"
DEFAULT_CALLGRAPH_DEPTH = 128
DEFAULT_ANNOTATE_SYMBOL_LIMIT = 10
