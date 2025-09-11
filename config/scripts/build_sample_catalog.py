# build_sample_catalog.py
from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import logging
import os
import sys
from functools import lru_cache
from pathlib import Path
from datetime import datetime, timezone
import xml.etree.ElementTree as ET

# Inlined dependencies from former sample_processing module
import re
import subprocess
import shutil
import uproot

SCHEMA_VERSION = "1.0"

logging.basicConfig(level=logging.INFO, format="%(levelname)s | %(message)s")

# ---- small helpers -----------------------------------------------------------

TOKEN_MAP_STAGE = {
    "selection_beam": "sel-beam",
    "selection_ext": "sel-ext",
    "selection_strangeness": "sel-strange",
    "selection_dirt": "sel-dirt",
}

def norm_run(run: str) -> str:
    digits = "".join(ch for ch in run if ch.isdigit())
    return f"r{digits}" if digits else run

def split_beam_key(beam_key: str) -> tuple[str, str]:
    # 'bnb' -> ('bnb','data'); 'bnb_ext' -> ('bnb','ext'); 'numi_fhc' -> ('numi','fhc'), etc.
    if "_" in beam_key:
        b, m = beam_key.split("_", 1)
        return b, m
    return beam_key, "data"

def infer_subset(sample_key: str, sample_type: str) -> str:
    key = (sample_key or "").lower()
    if "strange" in key:
        return "strange"
    if "dirt" in key or sample_type == "dirt":
        return "dirt"
    return "inc"

def dataset_id(beamline: str, mode: str, run: str, origin: str, subset: str, stage_name: str, detvar: str | None = None) -> str:
    parts = [beamline, mode, norm_run(run), origin, subset, TOKEN_MAP_STAGE.get(stage_name, stage_name)]
    if detvar:
        parts.append(detvar)
    return ".".join(parts)

def sha7(path: Path) -> str:
    h = hashlib.blake2s(digest_size=8)
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()[:7]

def version_tag() -> str:
    return datetime.now(timezone.utc).strftime("v%Y%m%d")

def runset_token(runs: list[str]) -> str:
    if not runs:
        return "all"
    if len(runs) == 1:
        return norm_run(runs[0])
    try:
        nums = sorted(int("".join(ch for ch in r if ch.isdigit())) for r in runs)
        if nums == list(range(nums[0], nums[-1] + 1)):
            return f"r{nums[0]}-r{nums[-1]}"
    except Exception:
        pass
    return ",".join(norm_run(r) for r in runs)

def summarize_beams_for_name(beam_keys: list[str]) -> str:
    beams = set(beam_keys)
    has_bnb = any(k.startswith("bnb") for k in beams)
    has_nu  = any(k.startswith("numi") for k in beams)
    if has_bnb and has_nu: return "nu+bnb"
    if has_bnb: return "bnb"
    if has_nu:
        pols = sorted({k.split("_", 1)[1] for k in beams if "_" in k and k != "numi_ext"})
        pol_str = f"[{','.join(pols)}]" if pols else ""
        return f"nu{pol_str}"
    return "multi"

# ---- sample processing utilities ---------------------------------------------

def get_xml_entities(xml_path: Path, content: str | None = None) -> dict[str, str]:
    if content is None:
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

@lru_cache(maxsize=256)
def list_root_files(input_dir: str) -> tuple[str, ...]:
    return tuple(str(p) for p in Path(input_dir).rglob("*.root"))

_POT_CACHE: dict[tuple[str, float], float] = {}

def get_total_pot_from_single_file(file_path: Path) -> float:
    if not file_path or not file_path.is_file():
        return 0.0
    try:
        st = file_path.stat()
        key = (str(file_path), st.st_mtime)
        if key in _POT_CACHE:
            return _POT_CACHE[key]
        tree_path = f"{file_path}:nuselection/SubRun"
        with uproot.open(tree_path) as tree:
            arr = tree["pot"].array(library="np")
            pot = float(arr.sum())
        _POT_CACHE[key] = pot
        return pot
    except Exception as exc:
        print(f"    Warning: Could not read POT from {file_path}: {exc}", file=sys.stderr)
        return 0.0

def get_total_pot_from_files_parallel(file_paths: list[str], max_workers: int) -> float:
    if not file_paths:
        return 0.0
    paths = [Path(p) for p in file_paths]
    mw = min(len(paths), max_workers)
    with concurrent.futures.ThreadPoolExecutor(max_workers=mw) as ex:
        return sum(ex.map(get_total_pot_from_single_file, paths))

def resolve_input_dir(stage_name: str | None, stage_outdirs: dict, entities: dict) -> str | None:
    return stage_outdirs.get(stage_name) if stage_name else None

def process_sample_entry(
    entry: dict,
    processed_analysis_path: Path,
    stage_outdirs: dict,
    entities: dict,
    run_pot: float,
    ext_triggers: int,
    jobs: int,
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

    root_files = list(list_root_files(input_dir))
    if not root_files:
        print(f"    Warning: No ROOT files found in {input_dir}. HADD will be skipped.", file=sys.stderr)

    if root_files and execute_hadd:
        if not run_command(["hadd", "-f", str(output_file), *root_files], execute_hadd):
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

    if sample_type == "mc" or is_detvar:
        entry["pot"] = get_total_pot_from_files_parallel(source_files, jobs)
        if entry["pot"] == 0.0:
            entry["pot"] = run_pot
    elif sample_type in {"data", "ext"}:
        entry["pot"] = run_pot
        entry["triggers"] = ext_triggers

    entry.pop("stage_name", None)
    return True

def load_xml_context(xml_paths: list[Path]) -> tuple[dict[str, str], dict[str, str]]:
    entities: dict[str, str] = {}
    stage_outdirs: dict[str, str] = {}
    for xml in xml_paths:
        text = xml.read_text()
        entities.update(get_xml_entities(xml, text))
        root = ET.fromstring(text)
        proj = root.find("project")
        if proj is None:
            logging.error("Could not find <project> in XML '%s'", xml)
            continue
        for s in proj.findall("stage"):
            outdir_text = s.findtext("outdir") or ""
            for name, val in entities.items():
                outdir_text = outdir_text.replace(f"&{name};", val)
            stage_outdirs[s.get("name")] = outdir_text
    return entities, stage_outdirs

def default_xmls() -> list[Path]:
    return [
        Path("/exp/uboone/app/users/nlane/production/strangeness_mcc9/srcs/ubana/ubana/searchingforstrangeness/xml/numi_fhc_workflow_core.xml"),
        Path("/exp/uboone/app/users/nlane/production/strangeness_mcc9/srcs/ubana/ubana/searchingforstrangeness/xml/numi_fhc_workflow_detvar.xml"),
    ]

# ---- core --------------------------------------------------------------------

def main() -> None:
    repo_root = Path(__file__).resolve().parents[1]

    ap = argparse.ArgumentParser(description="Aggregate ROOT samples from a recipe into a catalog.")
    ap.add_argument("--recipe", type=Path, required=True, help="Path to recipe JSON (instance).")
    ap.add_argument("--runs", default="", help='Comma-separated runs to process (e.g. "run1,run2"). Empty = all.')
    ap.add_argument("--outdir", type=Path, default=repo_root / "catalogs", help="Directory to write the catalog.")
    ap.add_argument("--xml", type=Path, nargs="*", default=None, help="Workflow XML files to parse (entities/outdirs).")
    ap.add_argument("--project", default=None, help="Project slug for filename; defaults to recipe['project'].")
    ap.add_argument(
        "--jobs",
        type=int,
        default=min(8, os.cpu_count() or 1),
        help="Max threads for POT aggregation",
    )
    args = ap.parse_args()

    recipe_path = args.recipe
    with open(recipe_path) as f:
        cfg = json.load(f)

    # Hard guards
    if cfg.get("role") != "recipe":
        sys.exit(f"Expected role='recipe', found '{cfg.get('role')}'.")
    if cfg.get("recipe_kind", "instance") == "template":
        sys.exit("Refusing to run on a template. Copy it and set recipe_kind='instance'.")

    project = args.project or cfg.get("project", "proj")
    runs_to_process = [r.strip() for r in args.runs.split(",") if r.strip()]
    produced_at = datetime.now(timezone.utc).isoformat(timespec="seconds")
    tag = version_tag()
    sha = sha7(recipe_path)

    xml_paths = args.xml if args.xml else default_xmls()
    entities, stage_outdirs = load_xml_context(xml_paths)

    ntuple_dir = Path(cfg["ntuple_base_directory"])
    ntuple_dir.mkdir(parents=True, exist_ok=True)

    beams_in: dict = cfg.get("run_configurations", {})
    runs_out: dict = {}

    for beam_key, run_block in beams_in.items():
        beam_active = bool(run_block.get("active", True))  # <-- beam-level switch (default: True)
        if not beam_active:
            logging.info("Skipping beam '%s' (active=false).", beam_key)
            continue

        beamline, mode = split_beam_key(beam_key)
        for run, run_details in run_block.items():
            if run == "active":
                continue  # not a run
            if runs_to_process and run not in runs_to_process:
                continue

            logging.info("Processing %s:%s", beam_key, run)

            is_ext = (mode == "ext") or beam_key.endswith("_ext")
            pot = float(run_details.get("pot", 0.0)) if not is_ext else 0.0
            ext_trig = int(run_details.get("ext_triggers", 0)) if is_ext else 0

            if not is_ext and pot == 0.0:
                logging.warning("No POT provided for %s:%s (on-beam).", beam_key, run)

            samples_in = run_details.get("samples", []) or []
            samples_out = []
            for sample in samples_in:
                s = dict(sample)

                origin = s.get("sample_type", "unknown")
                subset = infer_subset(s.get("sample_key", ""), origin)
                stage_name = s.get("stage_name", "selection_beam")
                s["dataset_id"] = dataset_id(beamline, mode, run, origin, subset, stage_name, None)

                ok = process_sample_entry(
                    s,
                    ntuple_dir,
                    stage_outdirs,
                    entities,
                    pot,
                    ext_trig,
                    args.jobs,
                )

                # detector variations (inherit processing)
                if ok and "detector_variations" in s:
                    new_vars = []
                    for dv in s["detector_variations"]:
                        dv2 = dict(dv)
                        detvar = dv2.get("variation_type")
                        dv2["dataset_id"] = dataset_id(beamline, mode, run, origin, subset, stage_name, detvar)
                        process_sample_entry(
                            dv2,
                            ntuple_dir,
                            stage_outdirs,
                            entities,
                            pot,
                            ext_trig,
                            args.jobs,
                            is_detvar=True,
                        )
                        new_vars.append(dv2)
                    s["detector_variations"] = new_vars

                samples_out.append(s)

            # write back run block
            run_copy = dict(run_details)
            run_copy["samples"] = samples_out
            runs_out.setdefault(beam_key, {})[run] = run_copy

    # emit catalog
    outdir = args.outdir
    outdir.mkdir(parents=True, exist_ok=True)

    beam_scope = summarize_beams_for_name(list(runs_out.keys()))
    runs_token = runset_token(runs_to_process)
    out_name = f"{project}.{beam_scope}.{runs_token}.catalog.{tag}-g{sha}.json"
    out_path = outdir / out_name

    catalog = {
        "role": "catalog",
        "schema_version": SCHEMA_VERSION,
        "produced_at": produced_at,
        "source_recipe_path": str(recipe_path),
        "source_recipe_hash": sha,
        "samples": {
            "ntupledir": cfg["ntuple_base_directory"],
            "run_configurations": runs_out,
        },
    }

    with open(out_path, "w") as f:
        json.dump(catalog, f, indent=4)

    logging.info("Wrote catalog: %s", out_path)

if __name__ == "__main__":
    main()
