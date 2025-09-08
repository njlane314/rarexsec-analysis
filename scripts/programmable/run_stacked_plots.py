#!/usr/bin/env python3
"""Run the analysis runner using preset-based specifications only.

This script builds a minimal pipeline composed of region, variable,
and plot presets and executes the analysis runner. The path to the
sample configuration is set to ``configs/sample.json`` at the
repository root so the example uses the shared configuration file.
"""

import json
import os
import subprocess
from typing import Optional, Dict, Any

ROOT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(ROOT_DIR, "..", ".."))
SAMPLE_CFG = os.path.join(REPO_ROOT, "config", "sample.json")


def build_pipeline(preset: str, preset_vars: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    block: Dict[str, Any] = {"name": preset, "kind": "preset"}
    if preset_vars:
        block["vars"] = preset_vars
    return {
        "pipeline": {
            "presets": [
                {"name": "TRUE_NEUTRINO_VERTEX", "kind": "region"},
                {"name": "RECO_NEUTRINO_VERTEX", "kind": "region"},
                {"name": "EMPTY", "kind": "variable"},
                block,
            ]
        }
    }


def run(preset: str = "STACKED_PLOTS", preset_vars: Optional[Dict[str, Any]] = None) -> None:
    pipeline = build_pipeline(preset, preset_vars)
    pipeline_path = os.path.join(ROOT_DIR, "pipeline.json")
    with open(pipeline_path, "w", encoding="utf-8") as handle:
        json.dump(pipeline, handle, indent=2)

    subprocess.run(
        ["./build/analyse", SAMPLE_CFG, pipeline_path],
        check=True,
    )


if __name__ == "__main__":
    run()
