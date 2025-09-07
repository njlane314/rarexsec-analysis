from __future__ import annotations

import concurrent.futures
import re
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
import uproot

from trigger_counts import get_trigger_counts_from_files_parallel

TRIGGER_BRANCH_MAP: dict[str, dict[str, str]] = {
    "numi": {"run1": "software_trigger"},
    "bnb": {"run1": "software_trigger"},
}

def select_trigger_count(trigger_counts: dict[str, int], beam: str, run: str) -> int:
    beam_key = "numi" if "numi" in beam.lower() else "bnb"
    branch = TRIGGER_BRANCH_MAP.get(beam_key, {}).get(run)
    if branch and branch in trigger_counts:
        return trigger_counts[branch]
    return next(iter(trigger_counts.values()), 0)

def get_xml_entities(xml_path: Path) -> dict[str, str]:
    content = xml_path.read_text()
    entity_regex = re.compile(r"<!ENTITY\s+([^\s]+)\s+\"([^\"]+)\">")
    return {match.group(1): match.group(2) for match in entity_regex.finditer(content)}

def run_command(command: list[str], execute: bool) -> bool:
    print(f"[COMMAND] {' '.join(command)}")
    if not execute:
        print("[INFO] Dry run mode. HADD command not executed.")
        return True

    try:
        subprocess.run(command, check=True)
        print("[STATUS] HADD Execution successful.")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError) as exc:
        print(f"[ERROR] HADD Execution failed: {exc}", file=sys.stderr)
        return False

def get_total_pot_from_single_file(file_path: Path) -> float:
    if not file_path or not file_path.is_file():
        return 0.0
    try:
        with uproot.open(file_path) as root_file:
            if "nuselection/SubRun" not in root_file:
                return 0.0
            subrun_tree = root_file["nuselection/SubRun"]
            if "pot" in subrun_tree:
                return float(subrun_tree["pot"].array(library="np").sum())
    except Exception as exc:
        print(f"    Warning: Could not read POT from {file_path}: {exc}", file=sys.stderr)
    return 0.0

def get_total_pot_from_files_parallel(file_paths: list[str]) -> float:
    with concurrent.futures.ThreadPoolExecutor() as executor:
        return sum(executor.map(get_total_pot_from_single_file, map(Path, file_paths)))

def resolve_input_dir(stage_name: str | None, stage_outdirs: dict, entities: dict) -> str | None:
    if not stage_name or stage_name not in stage_outdirs:
        return None
    input_dir = stage_outdirs[stage_name]
    for name, value in entities.items():
        input_dir = input_dir.replace(f"&{name};", value)
    return input_dir

def _collect_run_subrun_pairs(file_path: Path) -> set[tuple[int, int]]:
    pairs: set[tuple[int, int]] = set()
    try:
        with uproot.open(file_path) as root_file:
            if "nuselection/SubRun" not in root_file:
                return pairs
            tree = root_file["nuselection/SubRun"]
            runs = tree["run"].array(library="np") if "run" in tree else None
            subruns = tree["sub"].array(library="np") if "sub" in tree else None
            if runs is not None and subruns is not None:
                for r, s in zip(runs, subruns):
                    pairs.add((int(r), int(s)))
    except Exception as exc:
        print(f"    Warning: Could not read run/subrun from {file_path}: {exc}", file=sys.stderr)
    return pairs

def _get_pot_from_getdatainfo(tmp_path: Path, sample_type: str) -> float:
    script_path = Path("/exp/uboone/app/users/guzowski/slip_stacking/getDataInfo.py")
    command = [str(script_path), "-v3", "--format-numi", "--prescale", "--run-subrun-list", str(tmp_path)]
    if sample_type == "data":
        command.append("--slip")
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        lines = result.stdout.strip().splitlines()
        if len(lines) < 2:
            return 0.0
        header = lines[-2].split()
        values = lines[-1].split()
        pot_fields = [
            "tortgt_wcut",
            "tor875_wcut",
            "tor101_wcut",
            "tortgt",
            "tor875",
            "tor101",
        ]
        for field in pot_fields:
            if field in header:
                idx = header.index(field)
                try:
                    return float(values[idx])
                except (ValueError, IndexError):
                    return 0.0
        return 0.0
    except Exception as exc:
        print(f"    Warning: getDataInfo.py failed: {exc}", file=sys.stderr)
        return 0.0

def get_data_ext_pot_from_files(file_paths: list[str], sample_type: str) -> float:
    pairs: set[tuple[int, int]] = set()
    for file_path in map(Path, file_paths):
        pairs.update(_collect_run_subrun_pairs(file_path))
    if not pairs:
        return 0.0
    with tempfile.NamedTemporaryFile("w", delete=False) as tmp:
        for run, subrun in sorted(pairs):
            tmp.write(f"{run} {subrun}\n")
        tmp_path = Path(tmp.name)
    try:
        return _get_pot_from_getdatainfo(tmp_path, sample_type)
    finally:
        try:
            tmp_path.unlink()
        except OSError:
            pass

def process_sample_entry(
    entry: dict,
    processed_analysis_path: Path,
    stage_outdirs: dict,
    entities: dict,
    nominal_pot: float,
    beam: str,
    run: str,
    is_detvar: bool = False,
) -> bool:
    execute_hadd = entry.pop("do_hadd", False)

    if not entry.get("active", True):
        sample_key = entry.get("sample_key", "UNKNOWN")
        print(f"  Skipping {'detector variation' if is_detvar else 'sample'}: {sample_key} (marked as inactive)")
        return False

    stage_name = entry.get("stage_name")
    sample_key = entry.get("sample_key")
    sample_type = entry.get("sample_type", "mc")

    print(f"  Processing {'detector variation' if is_detvar else 'sample'}: {sample_key} (from stage: {stage_name})")
    print(
        f"    HADD execution for this {'sample' if not is_detvar else 'detector variation'}: {'Enabled' if execute_hadd else 'Disabled'}"
    )

    input_dir = resolve_input_dir(stage_name, stage_outdirs, entities)
    if not input_dir:
        print(
            f"    Warning: Stage '{stage_name}' not found in XML outdirs. Skipping {'detector variation' if is_detvar else 'sample'} '{sample_key}'.",
            file=sys.stderr,
        )
        return False

    output_file = processed_analysis_path / f"{sample_key}.root"
    entry["relative_path"] = output_file.name

    root_files = sorted(str(f) for f in Path(input_dir).rglob("*.root"))
    if not root_files:
        print(f"    Warning: No ROOT files found in {input_dir}. HADD will be skipped.", file=sys.stderr)

    if root_files and execute_hadd:
        if not run_command(["hadd", "-f", str(output_file), *root_files], True):
            print(
                f"    Error: HADD failed for {sample_key}. Skipping further processing for this entry.",
                file=sys.stderr,
            )
            return False
    elif not root_files and execute_hadd:
        print(
            f"    Note: No ROOT files found for '{sample_key}'. Skipping HADD but proceeding to record metadata (if applicable)."
        )
    elif root_files and not execute_hadd:
        print(f"    Note: HADD not requested for '{sample_key}'. Skipping HADD command.")

    source_files = [str(output_file)] if execute_hadd else root_files

    trigger_counts = get_trigger_counts_from_files_parallel(source_files)
    selected_triggers = select_trigger_count(trigger_counts, beam, run)

    if sample_type == "mc" or is_detvar:
        entry["pot"] = get_total_pot_from_files_parallel(source_files)
        entry["triggers"] = selected_triggers
        if entry["pot"] == 0.0:
            entry["pot"] = nominal_pot
    elif sample_type in {"data", "ext"}:
        entry["pot"] = get_data_ext_pot_from_files(source_files, sample_type)
        entry["triggers"] = selected_triggers
        if entry["pot"] == 0.0:
            entry["pot"] = nominal_pot

    entry.pop("stage_name", None)
    return True
