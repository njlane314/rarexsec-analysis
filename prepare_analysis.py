import xml.etree.ElementTree as ET
import argparse
import sys
from pathlib import Path
import subprocess
import re
import json

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
    if not file_path or not file_path.is_file(): return 0.0
    try:
        with uproot.open(file_path) as file:
            if "nuselection/SubRun" in file and "pot" in file["nuselection/SubRun"]:
                return float(file["nuselection/SubRun"]["pot"].array(library="np").sum())
    except Exception: return 0.0
    return 0.0

def get_total_triggers(file_path: Path) -> int:
    if not file_path or not file_path.is_file(): return 0
    try:
        with uproot.open(file_path) as file:
            if "nuselection/EventSelectionFilter" in file:
                return int(file["nuselection/EventSelectionFilter"].num_entries)
    except Exception: return 0
    return 0

def main():
    parser = argparse.ArgumentParser(description="A seamless workflow tool to HADD grid outputs and generate the analysis config.json.", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--master-config", required=True, help="Path to the master JSON configuration file.")
    parser.add_argument("--xml-file", required=True, help="Path to the project.xml file.")
    parser.add_argument("--output-config", default="config.json", help="Name of the final generated config file.")
    parser.add_argument("--execute-hadd", action="store_true", help="Actually execute the hadd commands. Default is a dry run.")
    args = parser.parse_args()

    # --- Part 1: Parse All necessary configurations ---
    print("===== PART 1: Loading Configurations =====")
    master_config_path = Path(args.master_config)
    xml_path = Path(args.xml_file)

    with open(master_config_path) as f:
        config = json.load(f)

    entities = get_xml_entities(xml_path)
    tree = ET.parse(xml_path)
    project_node = tree.find("project")
    if project_node is None:
        print("Error: Could not find <project> tag in XML file.", file=sys.stderr)
        sys.exit(1)

    stage_outdirs = {s.get("name"): s.find("outdir").text for s in project_node.findall("stage") if s.find("outdir") is not None}

    # --- Part 2: Process each sample defined in the master config ---
    print("\n===== PART 2: Hadding Files and Calculating Metadata =====")
    hadd_output_dir = Path(config["hadd_output_directory"])
    hadd_output_dir.mkdir(parents=True, exist_ok=True)
    config["ntuple_base_directory"] = str(hadd_output_dir.resolve())


    for beam, run_configs in config.get("run_configurations", {}).items():
        for run, run_details in run_configs.items():
            print(f"\nProcessing run: {run}")
            
            # Determine the reference POT for this run
            reference_pot = run_details.get("target_pot", 0.0)
            print(f"  Using target POT for this run: {reference_pot:.4e}")
            if reference_pot == 0.0:
                 print(f"  Warning: No target_pot specified for run '{run}'. MC scaling may be incorrect.", file=sys.stderr)


            for sample in run_details.get("samples", []):
                stage_name = sample.get("stage_name")
                sample_key = sample.get("sample_key")
                print(f"  Processing sample: {sample_key} (from stage: {stage_name})")

                if not stage_name or stage_name not in stage_outdirs:
                    print(f"    Warning: Stage '{stage_name}' not found in XML outdirs. Skipping sample '{sample_key}'.", file=sys.stderr)
                    continue

                # Construct paths and hadd the files
                input_dir_template = stage_outdirs[stage_name]
                input_dir = input_dir_template
                for name, value in entities.items():
                    input_dir = input_dir.replace(f"&{name};", value)
                
                output_file = hadd_output_dir / f"{sample_key}.root"
                sample["relative_path"] = output_file.name

                root_files = sorted([str(f) for f in Path(input_dir).rglob("*.root")])
                if not root_files:
                    print(f"    Warning: No ROOT files found in {input_dir}. HADD will be skipped.", file=sys.stderr)
                
                command = ["hadd", "-f", str(output_file)] + root_files
                if not run_command(command, args.execute_hadd):
                    continue # Skip to next sample if hadd fails
                
                # Calculate metadata from the NEWLY CREATED hadd file
                if sample["sample_type"] == "mc":
                    sample["pot"] = get_total_pot(output_file)
                elif sample["sample_type"] == "data":
                    sample["pot"] = reference_pot # Set data POT to the run's reference POT
                    sample["triggers"] = get_total_triggers(output_file)
                
                del sample["stage_name"] # Clean up the final config

            if "target_pot" in run_details:
                del run_details["target_pot"]

    # --- Part 3: Write Final Config ---
    output_path = Path(args.output_config)
    with open(output_path, "w") as f:
        json.dump(config, f, indent=4)
        
    print(f"\n--- Workflow Complete ---")
    print(f"Successfully generated analysis configuration at '{output_path}'")

if __name__ == "__main__":
    main()