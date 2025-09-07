from __future__ import annotations

import concurrent.futures
import sys
from pathlib import Path

import numpy as np
import uproot

TRIGGER_BRANCHES = [
    "software_trigger",
    "software_trigger_post",
    "software_trigger_pre",
    "software_trigger_post_ext",
    "software_trigger_pre_ext",
]

def get_trigger_counts_from_single_file(file_path: Path) -> dict[str, int]:
    counts: dict[str, int] = {}
    if not file_path or not file_path.is_file():
        return counts
    try:
        with uproot.open(file_path) as root_file:
            tree_path = "nuselection/EventSelectionFilter"
            if tree_path not in root_file:
                return counts
            tree = root_file[tree_path]
            for branch in TRIGGER_BRANCHES:
                if branch in tree.keys():
                    data = tree[branch].array(library="np")
                    if branch == "software_trigger":
                        counts[branch] = int(data.sum())
                    else:
                        counts[branch] = int((data != 0).sum())
    except Exception as exc:
        print(f"    Warning: Could not read trigger counts from {file_path}: {exc}", file=sys.stderr)
    return counts

def get_trigger_counts_from_files_parallel(file_paths: list[str]) -> dict[str, int]:
    totals: dict[str, int] = {}
    with concurrent.futures.ThreadPoolExecutor() as executor:
        for counts in executor.map(get_trigger_counts_from_single_file, map(Path, file_paths)):
            for branch, value in counts.items():
                totals[branch] = totals.get(branch, 0) + value
    return totals
