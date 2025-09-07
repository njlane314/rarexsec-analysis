from __future__ import annotations

import json
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

from sample_processing import get_xml_entities, process_sample_entry

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

def get_xml_entities(xml_path: Path) -> dict:
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
            if "nuselection/SubRun" in root_file:
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
    if not stage_name:
        return None

    input_dir = stage_outdirs.get(stage_name)
    if not input_dir:
        return None

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
    elif sample_type == "data":
        entry["pot"] = nominal_pot
        entry["triggers"] = selected_triggers

    entry.pop("stage_name", None)
    return True

def main() -> None:
    DEFINITIONS_PATH = "config/data.json"
    XML_PATHS = [
        "/exp/uboone/app/users/nlane/production/strangeness_mcc9/srcs/ubana/ubana/searchingforstrangeness/xml/numi_fhc_workflow_core.xml",
        "/exp/uboone/app/users/nlane/production/strangeness_mcc9/srcs/ubana/ubana/searchingforstrangeness/xml/numi_fhc_workflow_detvar.xml",
    ]
    CONFIG_PATH = "config/samples.json"
    RUNS_PROCESS = ["run1"]

    input_definitions_path = Path(DEFINITIONS_PATH)

    with open(input_definitions_path) as f:
        config = json.load(f)

    entities: dict[str, str] = {}
    stage_outdirs: dict[str, str] = {}
    for xml in XML_PATHS:
        xml_path = Path(xml)
        entities.update(get_xml_entities(xml_path))
        tree = ET.parse(xml_path)
        project_node = tree.find("project")
        if project_node is None:
            print(f"Error: Could not find <project> tag in XML file '{xml}'.", file=sys.stderr)
            continue
        stage_outdirs.update(
            {
                s.get("name"): s.find("outdir").text
                for s in project_node.findall("stage")
                if s.find("outdir") is not None
            }
        )

    processed_analysis_path = Path(config["ntuple_base_directory"])
    processed_analysis_path.mkdir(parents=True, exist_ok=True)

    for beam, run_configs in config.get("run_configurations", {}).items():
        for run, run_details in run_configs.items():
            if RUNS_PROCESS and run not in RUNS_PROCESS:
                print(f"\nSkipping run: {run} (not in specified RUNS_PROCESS list)")
                continue

            print(f"\nProcessing run: {run}")

            nominal_pot = run_details.get("nominal_pot", 0.0)

            if nominal_pot == 0.0:
                print(
                    f"  Warning: No nominal_pot specified for run '{run}'. MC scaling might be incorrect.",
                    file=sys.stderr,
                )
            else:
                print(f"  Using nominal POT for this run: {nominal_pot:.4e}")

            for sample in run_details.get("samples", []):
                if process_sample_entry(sample, processed_analysis_path, stage_outdirs, entities, nominal_pot, beam, run):
                    if "detector_variations" in sample:
                        for detvar_sample in sample["detector_variations"]:
                            process_sample_entry(
                                detvar_sample,
                                processed_analysis_path,
                                stage_outdirs,
                                entities,
                                nominal_pot,
                                beam,
                                run,
                                is_detvar=True,
                            )

    output_path = Path(CONFIG_PATH)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    samples_cfg = {
        "samples": {
            "ntuple_directory": config["ntuple_base_directory"],
            "beamlines": config["run_configurations"],
        }
    }
    with open(output_path, "w") as f:
        json.dump(samples_cfg, f, indent=4)

    print(f"\n--- Workflow Complete ---")
    print(f"Successfully generated configuration at '{output_path}'")

if __name__ == "__main__":
    main()
