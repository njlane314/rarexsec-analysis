from __future__ import annotations

import concurrent.futures
import re
import subprocess
import sys
import shutil
from pathlib import Path

import numpy as np
import uproot


def get_pot_ext_from_summary(
    pot_summary: dict, run: str, beam: str, horn_current: str | None
) -> tuple[float, int]:
    """Extract POT and EXT trigger totals from the pot summary JSON."""
    run_number = None
    if run.startswith("run"):
        try:
            run_number = int(run[3:])
        except ValueError:
            pass
    if run_number is None:
        return 0.0, 0

    run_entry = next(
        (r for r in pot_summary.get("runs", []) if r.get("index") == run_number),
        None,
    )
    if not run_entry:
        return 0.0, 0

    beam_key = "BNB" if "bnb" in beam.lower() else "NuMI"
    beam_entry = run_entry.get("beams", {}).get(beam_key, {})

    ext_triggers = int(beam_entry.get("TOTAL", {}).get("EXT", 0))

    pot_entry = beam_entry.get("TOTAL", {})
    if beam_key == "NuMI":
        if horn_current == "fhc":
            pot_entry = beam_entry.get("FHC", pot_entry)
        elif horn_current == "rhc":
            pot_entry = beam_entry.get("RHC", pot_entry)

    pot = pot_entry.get("TorB_Target_wcut") or pot_entry.get("TorA_wcut") or 0.0
    return float(pot), ext_triggers

def get_xml_entities(xml_path: Path) -> dict[str, str]:
    content = xml_path.read_text()
    entity_regex = re.compile(r"<!ENTITY\s+([^\s]+)\s+\"([^\"]+)\">")
    return {match.group(1): match.group(2) for match in entity_regex.finditer(content)}

def run_command(command: list[str], execute: bool) -> bool:
    print(f"[COMMAND] {' '.join(command)}")
    if not execute:
        print("[INFO] Dry run mode. HADD command not executed.")
        return True

    if shutil.which(command[0]) is None:
        print(
            f"[ERROR] Command '{command[0]}' not found. Ensure ROOT is set up by running:\n"
            "             source /cvmfs/larsoft.opensciencegrid.org/products/common/etc/setups\n"
            "             setup root",
            file=sys.stderr,
        )
        return False

    try:
        subprocess.run(command, check=True)
        print("[STATUS] HADD Execution successful.")
        return True
    except subprocess.CalledProcessError as exc:
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

def process_sample_entry(
    entry: dict,
    processed_analysis_path: Path,
    stage_outdirs: dict,
    entities: dict,
    nominal_pot: float,
    beam: str,
    run: str,
    pot_summary: dict,
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

    beam_lower = beam.lower()
    horn_current: str | None = None
    if "fhc" in beam_lower:
        horn_current = "fhc"
    elif "rhc" in beam_lower:
        horn_current = "rhc"

    if sample_type == "mc" or is_detvar:
        entry["pot"] = get_total_pot_from_files_parallel(source_files)
        if entry["pot"] == 0.0:
            entry["pot"] = nominal_pot
    elif sample_type in {"data", "ext"}:
        pot, ext = get_pot_ext_from_summary(pot_summary, run, beam, horn_current)
        entry["pot"] = pot if pot else nominal_pot
        entry["ext_triggers_db"] = ext

    entry.pop("stage_name", None)
    return True
