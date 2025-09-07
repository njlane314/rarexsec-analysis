from __future__ import annotations

import json
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

from sample_processing import get_xml_entities, process_sample_entry

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
            "ntupledir": config["ntuple_base_directory"],
            "beamlines": config["run_configurations"],
            "run_configurations": config["run_configurations"],
        }
    }
    with open(output_path, "w") as f:
        json.dump(samples_cfg, f, indent=4)

    print(f"\n--- Workflow Complete ---")
    print(f"Successfully generated configuration at '{output_path}'")

if __name__ == "__main__":
    main()
