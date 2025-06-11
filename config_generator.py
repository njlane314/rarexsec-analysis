import xml.etree.ElementTree as ET
import sys
from pathlib import Path
import subprocess
import re
import json
import uproot
import numpy as np

def get_xml_entities(xml_path: Path) -> dict:
    entities = {}
    entity_regex = re.compile(r'<!ENTITY\s+([^\s]+)\s+"([^"]+)">')
    with open(xml_path, 'r') as f:
        content = f.read()
        for match in entity_regex.finditer(content):
            entities[match.group(1)] = match.group(2)
    return entities

def run_command(command: list, execute: bool):
    print("--------------------------------------------------")
    print(f"[COMMAND] {' '.join(command)}")
    print("--------------------------------------------------")
    if execute:
        try:
            subprocess.run(command, check=True)
            print("[STATUS] HADD Execution successful.")
            return True
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            print(f"[ERROR] HADD Execution failed: {e}", file=sys.stderr)
            return False
    else:
        print("[INFO] Dry run mode. HADD command not executed.")
        return True

def get_total_pot(file_path: Path) -> float:
    if not file_path or not file_path.is_file():
        return 0.0
    try:
        with uproot.open(file_path) as file:
            if "nuselection/SubRun" in file:
                subrun_tree = file["nuselection/SubRun"]
                if "pot" in subrun_tree:
                    total_pot = subrun_tree["pot"].array(library="np").sum()
                    return float(total_pot)
    except Exception as e:
        print(f"    Warning: Could not read POT from {file_path}: {e}", file=sys.stderr)
        return 0.0
    return 0.0

def get_total_triggers(file_path: Path) -> int:
    if not file_path or not file_path.is_file():
        return 0
    try:
        with uproot.open(file_path) as file:
            if "nuselection/EventSelectionFilter" in file:
                return int(file["nuselection/EventSelectionFilter"].num_entries)
    except Exception as e:
        print(f"    Warning: Could not read triggers from {file_path}: {e}", file=sys.stderr)
        return 0
    return 0

def process_sample_entry(entry: dict, processed_analysis_path: Path, stage_outdirs: dict, entities: dict, nominal_pot: float, execute_hadd: bool, is_detvar: bool = False):
    stage_name = entry.get("stage_name")
    sample_key = entry.get("sample_key")
    sample_type = entry.get("sample_type", "mc") 

    print(f"  Processing {'detector variation' if is_detvar else 'sample'}: {sample_key} (from stage: {stage_name})")

    if not stage_name or stage_name not in stage_outdirs:
        print(f"    Warning: Stage '{stage_name}' not found in XML outdirs. Skipping {'detector variation' if is_detvar else 'sample'} '{sample_key}'.", file=sys.stderr)
        return False

    input_dir_template = stage_outdirs[stage_name]
    input_dir = input_dir_template
    for name, value in entities.items():
        input_dir = input_dir.replace(f"&{name};", value)

    output_file = processed_analysis_path / f"{sample_key}.root"
    entry["relative_path"] = output_file.name

    root_files = sorted([str(f) for f in Path(input_dir).rglob("*.root")])
    if not root_files:
        print(f"    Warning: No ROOT files found in {input_dir}. HADD will be skipped.", file=sys.stderr)

    command = ["hadd", "-f", str(output_file)] + root_files
    if not run_command(command, execute_hadd) and execute_hadd:
        return False
    
    if sample_type == "mc" or is_detvar:
        entry["pot"] = get_total_pot(output_file)
        if entry["pot"] == 0.0:
            entry["pot"] = nominal_pot
    elif sample_type == "data":
        entry["pot"] = nominal_pot 
        entry["triggers"] = get_total_triggers(output_file)

    del entry["stage_name"]
    return True


def main():
    DEFINITIONS_PATH = "config_definitions.json"
    XML_PATH = "/exp/uboone/app/users/nlane/production/strangeness_mcc9/srcs/ubana/ubana/searchingforstrangeness/numi_fhc_workflow.xml"
    CONFIG_PATH = "config.json"
    EXECUTE_HADD = False
    RUNS_PROCESS = ["run1"]

    print("===== PART 1: Loading Configurations =====")
    input_definitions_path = Path(DEFINITIONS_PATH)
    xml_path = Path(XML_PATH)

    with open(input_definitions_path) as f:
        config = json.load(f)

    entities = get_xml_entities(xml_path)
    tree = ET.parse(xml_path)
    project_node = tree.find("project")
    if project_node is None:
        print("Error: Could not find <project> tag in XML file.", file=sys.stderr)
        sys.exit(1)

    stage_outdirs = {s.get("name"): s.find("outdir").text for s in project_node.findall("stage") if s.find("outdir") is not None}

    print("\n===== PART 2: Hadding Files and Calculating Metadata =====")
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
                print(f"  Warning: No nominal_pot specified for run '{run}'. MC scaling might be incorrect.", file=sys.stderr)
            else:
                print(f"  Using nominal POT for this run: {nominal_pot:.4e}")

            for sample in run_details.get("samples", []):
                process_sample_entry(sample, processed_analysis_path, stage_outdirs, entities, nominal_pot, EXECUTE_HADD)

                if "detector_variations" in sample:
                    for detvar_sample in sample["detector_variations"]:
                        process_sample_entry(detvar_sample, processed_analysis_path, stage_outdirs, entities, nominal_pot, EXECUTE_HADD, is_detvar=True)
            
    output_path = Path(CONFIG_PATH)
    with open(output_path, "w") as f:
        json.dump(config, f, indent=4)

    print(f"\n--- Workflow Complete ---")
    print(f"Successfully generated analysis configuration at '{output_path}'")

if __name__ == "__main__":
    main()