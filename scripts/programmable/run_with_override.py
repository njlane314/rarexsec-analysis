import json
import os
import subprocess
from typing import Dict, Any

ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
SAMPLE_CFG = os.path.join(ROOT_DIR, "sample.json")


def build_pipeline() -> Dict[str, Any]:
    return {
        "pipeline": {
            "presets": [
                {"name": "TRUE_NEUTRINO_VERTEX", "kind": "region"},
                {"name": "RECO_NEUTRINO_VERTEX", "kind": "region"},
                {"name": "EMPTY", "kind": "variable"},
                {
                    "name": "STACKED_PLOTS",
                    "kind": "preset",
                    "overrides": {
                        "StackedHistogramPlugin": {
                            "plot_configs": {"output_directory": "./alt_plots"}
                        }
                    },
                },
            ]
        }
    }


def run() -> None:
    pipeline = build_pipeline()
    pipeline_path = os.path.join(ROOT_DIR, "pipeline_override.json")
    with open(pipeline_path, "w", encoding="utf-8") as handle:
        json.dump(pipeline, handle, indent=2)

    subprocess.run(["./build/analyse", SAMPLE_CFG, pipeline_path], check=True)


if __name__ == "__main__":
    run()
