import json
import uproot
import argparse
import sys
from pathlib import Path

def get_total_pot(file_path: Path) -> float:
    if not file_path or not file_path.is_file():
        return 0.0
    try:
        with uproot.open(file_path) as file:
            if "nuselection/SubRun" not in file:
                return 0.0
            subrun_tree = file["nuselection/SubRun"]
            if "pot" in subrun_tree:
                return float(subrun_tree["pot"].array(library="np").sum())
            return 0.0
    except Exception:
        return 0.0

def get_total_triggers(file_path: Path) -> int:
    if not file_path or not file_path.is_file():
        return 0
    try:
        with uproot.open(file_path) as file:
            if "nuselection/EventSelectionFilter" in file:
                return int(file["nuselection/EventSelectionFilter"].num_entries)
            return 0
    except Exception:
        return 0

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--master", default="master_config.json", help="Path to the master configuration file.")
    parser.add_argument("--output", default="config.json", help="Path for the generated output configuration file.")
    args = parser.parse_args()

    master_config_path = Path(args.master)
    if not master_config_path.is_file():
        print(f"Error: Master configuration file not found at '{master_config_path}'", file=sys.stderr)
        sys.exit(1)
        
    with open(master_config_path) as f:
        config = json.load(f)

    base_dir = Path(config["ntuple_base_directory"])
    
    for beam, run_configs in config.get("run_configurations", {}).items():
        for run, run_details in run_configs.items():
            
            reference_pot = 0.0
            reference_triggers = 0
            
            data_sample_path = None
            for sample in run_details.get("samples", []):
                if sample.get("sample_type") == "data":
                    if sample.get("relative_path"):
                        data_sample_path = base_dir / sample["relative_path"]
                    break
            
            if data_sample_path and data_sample_path.is_file():
                reference_pot = get_total_pot(data_sample_path)
                reference_triggers = get_total_triggers(data_sample_path)
            
            if reference_pot == 0.0:
                reference_pot = run_details.get("target_pot", 0.0)
            if reference_triggers == 0:
                reference_triggers = run_details.get("target_triggers", 0)

            for sample in run_details.get("samples", []):
                sample_type = sample.get("sample_type")
                
                if sample_type == "data":
                    sample["pot"] = reference_pot
                    sample["triggers"] = reference_triggers
                
                elif sample_type == "mc":
                    if sample.get("relative_path"):
                        full_path = base_dir / sample["relative_path"]
                        sample["pot"] = get_total_pot(full_path)
                
                elif sample_type == "ext":
                    if sample.get("relative_path"):
                        full_path = base_dir / sample["relative_path"]
                        sample["triggers"] = get_total_triggers(full_path)
            
            if "target_pot" in run_details:
                del run_details["target_pot"]
            if "target_triggers" in run_details:
                del run_details["target_triggers"]

    output_path = Path(args.output)
    with open(output_path, "w") as f:
        json.dump(config, f, indent=4)
        
    print(f"Successfully generated configuration file at '{output_path}'")

if __name__ == "__main__":
    main()