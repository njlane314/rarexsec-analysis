#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
import xml.etree.ElementTree as ET
from typing import Iterable, Set, Tuple, Dict, Optional

import numpy as np
import uproot

# ---------------- XML helpers ----------------

ENTITY_RE = re.compile(r'<!ENTITY\s+([^\s]+)\s+"([^"]+)">')


def get_xml_entities(xml_path: Path) -> Dict[str, str]:
    txt = xml_path.read_text()
    return {m.group(1): m.group(2) for m in ENTITY_RE.finditer(txt)}


def resolve_stage_outdir(xml_path: Path, stage_name: str) -> Path:
    entities = get_xml_entities(xml_path)
    tree = ET.parse(xml_path)
    project = tree.find("project")
    if project is None:
        raise RuntimeError(f"Could not find <project> in {xml_path}")
    stage = next((s for s in project.findall("stage") if s.get("name") == stage_name), None)
    if stage is None:
        raise RuntimeError(f"Stage '{stage_name}' not found in {xml_path}")
    node = stage.find("outdir")
    if node is None or not (node.text or "").strip():
        raise RuntimeError(f"Stage '{stage_name}' has no <outdir> text in {xml_path}")
    outdir = node.text
    for k, v in entities.items():
        outdir = outdir.replace(f"&{k};", v)
    return Path(outdir).expanduser()

# ---------------- ROOT helpers ----------------

PREFERRED_SUBRUN_TREES = (
    "nuselection/SubRuns",
    "nuselection/SubRun",
    "SubRuns",
    "SubRun",
    "ana/SubRuns",
    "ana/SubRun",
)

PREFERRED_EVENT_TREES = (
    "nuselection/Events",
    "Events",
    "ana/Events",
)

# Accept lower-cased variants
RUN_KEYS = ("run", "runnumber", "run_number", "ub_run")
SUBRUN_KEYS = ("subrun", "sub_run", "subrunnumber", "subrun_number", "ub_subrun")


def _find_branch_name(ttree: uproot.behaviors.TBranch.HasBranches, wanted: tuple[str, ...]) -> Optional[str]:
    keys = list(ttree.keys())
    lower = {k.lower(): k for k in keys}
    for w in wanted:
        if w in lower:
            return lower[w]
    # allow branch names like "something.run"
    for k in keys:
        base = k.split(".")[-1].lower()
        if base in wanted:
            return k
    return None


def _extract_pairs_from_ttree(ttree) -> Set[Tuple[int, int]]:
    pairs: Set[Tuple[int, int]] = set()
    run_name = _find_branch_name(ttree, RUN_KEYS)
    subrun_name = _find_branch_name(ttree, SUBRUN_KEYS)
    if not run_name or not subrun_name:
        return pairs
    for chunk in ttree.iterate(filter_name=(run_name, subrun_name), step_size="50 MB", library="np"):
        run = np.asarray(chunk[run_name]).astype(int)
        sub = np.asarray(chunk[subrun_name]).astype(int)
        pairs |= set(zip(run.tolist(), sub.tolist()))
    return pairs


def extract_pairs_from_file(
    root_file: Path,
    show_structure: bool = False,
    force_tree: Optional[str] = None,
    force_run: Optional[str] = None,
    force_subrun: Optional[str] = None,
) -> Set[Tuple[int, int]]:
    pairs: Set[Tuple[int, int]] = set()
    try:
        with uproot.open(root_file) as rf:
            # Optional structure dump (first file only from main)
            if show_structure:
                classmap = rf.classnames()  # name -> class
                treenames = [n for n, cls in classmap.items() if cls.endswith("TTree")]
                print(f"[STRUCT] {root_file}")
                for n in sorted(treenames):
                    try:
                        t = rf[n]
                        k = list(t.keys())
                        preview = k[:15]
                        tail = "..." if len(k) > 15 else ""
                        print(f"   - {n}: branches={preview}{tail}")
                    except Exception as e:
                        print(f"   - {n}: <unreadable> ({e})")

            # Forced path (exact control)
            if force_tree and force_run and force_subrun and force_tree in rf:
                t = rf[force_tree]
                for chunk in t.iterate(filter_name=(force_run, force_subrun), step_size="50 MB", library="np"):
                    run = np.asarray(chunk[force_run]).astype(int)
                    sub = np.asarray(chunk[force_subrun]).astype(int)
                    pairs |= set(zip(run.tolist(), sub.tolist()))
                return pairs

            # 1) Preferred subrun trees
            for name in PREFERRED_SUBRUN_TREES:
                if name in rf:
                    t = rf[name]
                    got = _extract_pairs_from_ttree(t)
                    if got:
                        pairs |= got
                        return pairs

            # 2) Preferred event trees
            for name in PREFERRED_EVENT_TREES:
                if name in rf:
                    t = rf[name]
                    got = _extract_pairs_from_ttree(t)
                    if got:
                        pairs |= got
                        return pairs

            # 3) Discovery: any TTree with both run-like and subrun-like branches
            classmap = rf.classnames()
            treenames = [n for n, cls in classmap.items() if cls.endswith("TTree")]
            for name in treenames:
                try:
                    t = rf[name]
                    run_name = _find_branch_name(t, RUN_KEYS)
                    subrun_name = _find_branch_name(t, SUBRUN_KEYS)
                    if run_name and subrun_name:
                        got = _extract_pairs_from_ttree(t)
                        if got:
                            pairs |= got
                            return pairs
                except Exception as e:
                    print(f"[WARN] Skipping {root_file}::{name}: {e}", file=sys.stderr)

    except Exception as e:
        print(f"[WARN] Failed opening {root_file}: {e}", file=sys.stderr)

    return pairs


def iter_root_files(root_dir: Path) -> Iterable[Path]:
    yield from (p for p in root_dir.rglob("*.root") if p.is_file())

# ---------------- CLI ----------------

def main() -> None:
    ap = argparse.ArgumentParser(
        description="Scan selection_ext outputs and dump unique (run,subrun) pairs to CSV (no header)."
    )
    ap.add_argument(
        "--xml",
        default="/exp/uboone/app/users/nlane/production/strangeness_mcc9/srcs/ubana/ubana/searchingforstrangeness/xml/numi_fhc_workflow_core.xml",
        help="Path to workflow XML",
    )
    ap.add_argument("--stage", default="selection_ext", help="Stage name that produced the EXT files")
    ap.add_argument("-o", "--out", default="pairs.csv", help="Output CSV (no header)")
    ap.add_argument("--debug-structure", action="store_true", help="Print tree/branch structure for the first file")
    ap.add_argument("--tree", help="Force a TTree path to read (e.g. 'SubRuns' or 'nuselection/SubRun')")
    ap.add_argument("--run-branch", help="Override run branch name (e.g. 'run' or 'runnumber')")
    ap.add_argument("--subrun-branch", help="Override subrun branch name (e.g. 'subrun' or 'subrunnumber')")
    ap.add_argument("--print-first", type=int, default=0, help="Print first N pairs for a quick look")
    args = ap.parse_args()

    outdir = resolve_stage_outdir(Path(args.xml), args.stage)
    if not outdir.exists():
        print(f"[ERROR] Resolved outdir does not exist: {outdir}", file=sys.stderr)
        sys.exit(2)

    print(f"[INFO] Outdir: {outdir}")
    files = sorted(iter_root_files(outdir))
    if not files:
        print(f"[ERROR] No .root files under: {outdir}", file=sys.stderr)
        sys.exit(3)
    print(f"[INFO] Found {len(files)} ROOT files. Extracting (run,subrun)...")

    all_pairs: Set[Tuple[int, int]] = set()
    first = True
    for i, f in enumerate(files, 1):
        pairs = extract_pairs_from_file(
            f,
            show_structure=(args.debug_structure and first),
            force_tree=args.tree,
            force_run=args.run_branch,
            force_subrun=args.subrun_branch,
        )
        first = False
        all_pairs |= pairs
        if i % 50 == 0:
            print(f"  ... {i}/{len(files)} files, {len(all_pairs)} unique pairs so far")

    if not all_pairs:
        print("[ERROR] No (run,subrun) pairs found in any file. Try --debug-structure or --tree/--run-branch/--subrun-branch.", file=sys.stderr)
        sys.exit(4)

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w") as f:
        for r, s in sorted(all_pairs):
            f.write(f"{r},{s}\n")

    print(f"[OK] Unique pairs: {len(all_pairs)}")
    print(f"[OK] Wrote CSV (no header): {out_path}")

    if args.print_first > 0:
        print("[Preview]")
        for r, s in list(sorted(all_pairs))[: args.print_first]:
            print(f"{r},{s}")


if __name__ == "__main__":
    main()

