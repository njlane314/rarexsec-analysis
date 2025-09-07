from __future__ import annotations

import concurrent.futures
import os
import re
import subprocess
import sys
import tempfile
import shutil
from pathlib import Path

import numpy as np
import uproot

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

def _iter_ttrees(directory):
    """Yield all TTree objects within a ROOT file or directory."""
    for key in directory.keys():
        obj = directory[key]
        if isinstance(obj, uproot.behaviors.TTree.TTree):
            yield obj
        elif hasattr(obj, "keys"):
            yield from _iter_ttrees(obj)


def _collect_run_subrun_pairs(file_path: Path) -> set[tuple[int, int]]:
    pairs: set[tuple[int, int]] = set()
    try:
        with uproot.open(file_path) as root_file:
            for tree in _iter_ttrees(root_file):
                try:
                    runs = tree["run"].array(library="np")
                    subruns = tree["subRun"].array(library="np")
                except Exception:
                    continue
                pairs.update(zip(map(int, runs), map(int, subruns)))
                if pairs:
                    break
    except Exception as exc:
        print(f"    Warning: Could not read run/subrun from {file_path}: {exc}", file=sys.stderr)
    return pairs

def _get_pot_from_getdatainfo(
    tmp_path: Path, sample_type: str, beam: str, horn_current: str | None
) -> float:
    script_path = Path("/exp/uboone/app/users/guzowski/slip_stacking/getDataInfo.py")

    beam_lower = beam.lower()
    if "numi" in beam_lower:
        format_flag = "--format-numi"
        pot_fields = ["tortgt_wcut", "tor101_wcut", "tortgt", "tor101"]
        header_keys = ["EXT", "Gate1", "tortgt_wcut"]
    elif "bnb" in beam_lower:
        format_flag = "--format-bnb"
        pot_fields = ["tor875_wcut", "tor860_wcut", "tor875", "tor860"]
        header_keys = ["EXT", "Gate2", "tor875_wcut"]
    else:
        print(f"    Error: Unknown beam type '{beam}'", file=sys.stderr)
        return 0.0

    python2 = shutil.which("python2")
    if python2 is None:
        print(
            "    Warning: python2 interpreter not found; attempting to run getDataInfo.py directly",
            file=sys.stderr,
        )
        command = [
            str(script_path),
            "-v3",
            format_flag,
            "--prescale",
            "--run-subrun-list",
            str(tmp_path),
        ]
    else:
        command = [
            python2,
            str(script_path),
            "-v3",
            format_flag,
            "--prescale",
            "--run-subrun-list",
            str(tmp_path),
        ]

    if "numi" in beam_lower:
        command.append("--horncurr")
    if sample_type == "data":
        command.append("--slip")

    print(f"[COMMAND] {' '.join(command)}")
    env = os.environ.copy()
    env.pop("PYTHONHOME", None)
    env.pop("PYTHONPATH", None)
    if shutil.which("samweb") is None:
        print(
            "    Warning: 'samweb' command not found. Ensure SAM is set up by running:\n"
            "             source /cvmfs/fermilab.opensciencegrid.org/products/common/etc/setups\n"
            "             setup sam_web_client",
            file=sys.stderr,
        )
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True, env=env)
        lines = result.stdout.strip().splitlines()

        header_index = -1
        for i, line in enumerate(lines):
            if all(key in line for key in header_keys):
                header_index = i
                break
        if header_index == -1:
            print(
                "    Warning: Could not find header line in getDataInfo.py output.",
                file=sys.stderr,
            )
            return 0.0

        header = lines[header_index].split()

        data_line: str | None = None
        if horn_current and "numi" in beam_lower:
            search_line_start = (
                "Forward Horn Current:" if horn_current == "fhc" else "Reverse Horn Current:"
            )
            for line in lines[header_index + 1 :]:
                if line.strip().startswith(search_line_start):
                    data_line = line
                    break
        if data_line is None:
            if header_index + 1 >= len(lines):
                print(
                    "    Warning: Found header but no subsequent data line.",
                    file=sys.stderr,
                )
                return 0.0
            values = lines[header_index + 1].split()
        else:
            values = re.findall(r"[-+]?\d+\.?\d*(?:[eE][-+]?\d+)?", data_line)

        for field in pot_fields:
            if field in header:
                idx = header.index(field)
                if idx < len(values):
                    try:
                        return float(values[idx])
                    except ValueError:
                        return 0.0
        print(
            "    Warning: POT field not found in getDataInfo.py output.",
            file=sys.stderr,
        )
        return 0.0
    except subprocess.CalledProcessError as exc:
        print(
            f"    Warning: getDataInfo.py failed with exit code {exc.returncode}",
            file=sys.stderr,
        )
        print(f"    stderr: {exc.stderr.strip()}", file=sys.stderr)
        return 0.0
    except Exception as exc:
        print(
            f"    Warning: An unexpected error occurred while running getDataInfo.py: {exc}",
            file=sys.stderr,
        )
        return 0.0

def get_data_ext_pot_from_files(
    file_paths: list[str], sample_type: str, beam: str, horn_current: str | None
) -> float:
    pairs: set[tuple[int, int]] = set()
    for file_path in map(Path, file_paths):
        pairs.update(_collect_run_subrun_pairs(file_path))
    if not pairs:
        print("    Warning: No run/subrun pairs found; skipping getDataInfo.py", file=sys.stderr)
        return 0.0
    with tempfile.NamedTemporaryFile("w", delete=False) as tmp:
        for run, subrun in sorted(pairs):
            tmp.write(f"{run} {subrun}\n")
        tmp_path = Path(tmp.name)
    try:
        return _get_pot_from_getdatainfo(tmp_path, sample_type, beam, horn_current)
    finally:
        try:
            tmp_path.unlink()
        except OSError:
            pass


def _get_ext_triggers_from_getdatainfo(tmp_path: Path, beam: str) -> int:
    """Execute getDataInfo.py and parse the EXT trigger count from its output."""
    script_path = Path("/exp/uboone/app/users/guzowski/slip_stacking/getDataInfo.py")

    beam_lower = beam.lower()
    if "numi" in beam_lower:
        format_flag = "--format-numi"
        header_keys = ["EXT", "Gate1"]
    elif "bnb" in beam_lower:
        format_flag = "--format-bnb"
        header_keys = ["EXT", "Gate2"]
    else:
        print(f"     Error: Unknown beam type '{beam}'", file=sys.stderr)
        return 0

    python2 = shutil.which("python2")
    if python2 is None:
        print(
            "     Warning: python2 interpreter not found; attempting to run getDataInfo.py directly",
            file=sys.stderr,
        )
        command = [
            str(script_path),
            "-v3",
            format_flag,
            "--run-subrun-list",
            str(tmp_path),
        ]
    else:
        command = [
            python2,
            str(script_path),
            "-v3",
            format_flag,
            "--run-subrun-list",
            str(tmp_path),
        ]

    try:
        env = os.environ.copy()
        env.pop("PYTHONHOME", None)
        env.pop("PYTHONPATH", None)
        if shutil.which("samweb") is None:
            print(
                "     Warning: 'samweb' command not found. Ensure SAM is set up by running:\n"
                "             source /cvmfs/fermilab.opensciencegrid.org/products/common/etc/setups\n"
                "             setup sam_web_client",
                file=sys.stderr,
            )
        result = subprocess.run(command, check=True, capture_output=True, text=True, env=env)
        lines = result.stdout.strip().splitlines()

        header_index = -1
        for i, line in enumerate(lines):
            if all(key in line for key in header_keys):
                header_index = i
                break

        if header_index == -1:
            print(
                "     Warning: Could not find header line in getDataInfo.py output.",
                file=sys.stderr,
            )
            return 0
        if header_index + 1 >= len(lines):
            print(
                "     Warning: Found header but no subsequent data line.",
                file=sys.stderr,
            )
            return 0

        header = lines[header_index].split()
        values = lines[header_index + 1].split()

        if "EXT" in header:
            idx = header.index("EXT")
            try:
                if idx < len(values):
                    return int(float(values[idx]))
                print(
                    "     Warning: Mismatch between header and data columns.",
                    file=sys.stderr,
                )
            except ValueError:
                print(
                    f"     Warning: Could not convert EXT value '{values[idx]}' to int.",
                    file=sys.stderr,
                )
            return 0

        print("     Warning: 'EXT' column not found in the header.", file=sys.stderr)
        return 0

    except subprocess.CalledProcessError as exc:
        print(
            f"     Warning: getDataInfo.py failed with exit code {exc.returncode}",
            file=sys.stderr,
        )
        print(f"     stderr: {exc.stderr.strip()}", file=sys.stderr)
        return 0
    except Exception as exc:
        print(
            f"     Warning: An unexpected error occurred while running getDataInfo.py: {exc}",
            file=sys.stderr,
        )
        return 0


def get_ext_triggers_from_files(file_paths: list[str], beam: str) -> int:
    pairs: set[tuple[int, int]] = set()
    for file_path in map(Path, file_paths):
        pairs.update(_collect_run_subrun_pairs(file_path))
    if not pairs:
        return 0
    with tempfile.NamedTemporaryFile("w", delete=False) as tmp:
        for run, subrun in sorted(pairs):
            tmp.write(f"{run} {subrun}\n")
        tmp_path = Path(tmp.name)
    try:
        return _get_ext_triggers_from_getdatainfo(tmp_path, beam)
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
        entry["pot"] = get_data_ext_pot_from_files(
            source_files, sample_type, beam, horn_current
        )
        entry["ext_triggers_db"] = get_ext_triggers_from_files(source_files, beam)
        if entry["pot"] == 0.0:
            entry["pot"] = nominal_pot

    entry.pop("stage_name", None)
    return True
