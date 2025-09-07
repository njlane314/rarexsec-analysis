#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import socket
import sqlite3
from datetime import datetime
from pathlib import Path
from typing import Iterable, Set, Tuple

import numpy as np
import uproot


DEFAULT_DBROOT = "/exp/uboone/data/uboonebeam/beamdb"
DEFAULT_RUN_DB = f"{DEFAULT_DBROOT}/run.db"


def iter_root_files(inputs: Iterable[str]) -> Iterable[Path]:
    for p in inputs:
        path = Path(p)
        if path.is_dir():
            yield from (f for f in path.rglob("*.root") if f.is_file())
        elif path.is_file() and path.suffix == ".root":
            yield path


def extract_pairs_from_file(f: Path) -> Set[Tuple[int, int]]:
    pairs: Set[Tuple[int, int]] = set()
    try:
        with uproot.open(f) as rf:
            # Prefer subrun-level trees first
            for tree_name in ("SubRuns", "nuselection/SubRun", "subrun", "subruns"):
                if tree_name in rf:
                    t = rf[tree_name]
                    if all(b in t.keys() for b in ("run", "subrun")):
                        run = t["run"].array(library="np")
                        sub = t["subrun"].array(library="np")
                        pairs |= set(zip(run.astype(int).tolist(), sub.astype(int).tolist()))
                        return pairs  # good enough; stop here

            # Fallback to Events (heavier, but robust)
            for tree_name in ("Events", "nuselection/Events"):
                if tree_name in rf:
                    t = rf[tree_name]
                    if all(b in t.keys() for b in ("run", "subrun")):
                        # Load as small as possible
                        run = t["run"].array(library="np")
                        sub = t["subrun"].array(library="np")
                        pairs |= set(zip(run.astype(int).tolist(), sub.astype(int).tolist()))
                        return pairs

    except Exception as e:
        print(f"[WARN] {f}: failed extracting (run,subrun): {e}")
    return pairs


def summarize_pairs(pairs: Set[Tuple[int, int]]) -> dict:
    by_run: dict[int, int] = {}
    for r, s in pairs:
        by_run[r] = by_run.get(r, 0) + 1
    return {"n_pairs": len(pairs), "by_run_counts": by_run}


def sum_ext_triggers(run_db: str, pairs: Set[Tuple[int, int]]) -> tuple[int, list[tuple[int, int]] , dict[int, int]]:
    if not pairs:
        return 0, [], {}

    conn = sqlite3.connect(run_db)
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()
    cur.execute("PRAGMA temp_store=MEMORY;")
    cur.execute("CREATE TEMP TABLE pairs(run INTEGER, subrun INTEGER);")
    cur.executemany("INSERT INTO pairs(run, subrun) VALUES (?, ?);", list(pairs))

    # Total EXTTrig
    total = cur.execute("""
        SELECT IFNULL(SUM(r.EXTTrig), 0)
        FROM runinfo r
        JOIN pairs p ON r.run = p.run AND r.subrun = p.subrun;
    """).fetchone()[0]

    # Missing (run,subrun) present in files but not in DB
    missing = cur.execute("""
        SELECT p.run, p.subrun
        FROM pairs p
        LEFT JOIN runinfo r ON r.run = p.run AND r.subrun = p.subrun
        WHERE r.run IS NULL;
    """).fetchall()
    missing_pairs = [(int(x["run"]), int(x["subrun"])) for x in missing]

    # By-run breakdown (handy sanity check)
    by_run_rows = cur.execute("""
        SELECT r.run AS run, IFNULL(SUM(r.EXTTrig), 0) AS ext_sum
        FROM runinfo r
        JOIN pairs p ON r.run = p.run AND r.subrun = p.subrun
        GROUP BY r.run
        ORDER BY r.run;
    """).fetchall()
    by_run = {int(r["run"]): int(r["ext_sum"]) for r in by_run_rows}

    conn.close()
    return int(total), missing_pairs, by_run


def main():
    ap = argparse.ArgumentParser(
        description="Compute external triggers (EXTTrig) for the (run,subrun) pairs found in EXT ROOT files."
    )
    ap.add_argument("inputs", nargs="+",
                    help="ROOT files and/or directories to scan recursively for *.root")
    ap.add_argument("-o", "--out", default="ext_triggers_summary.json",
                    help="Output JSON path (default: ext_triggers_summary.json)")
    ap.add_argument("--run-db", default=DEFAULT_RUN_DB,
                    help=f"Path to run.db (default: {DEFAULT_RUN_DB})")
    ap.add_argument("--value-only", action="store_true",
                    help="Print only the total ext triggers (integer) and exit with 0")
    args = ap.parse_args()

    root_files = list(iter_root_files(args.inputs))
    if not root_files:
        print("[ERROR] No ROOT files found in given inputs.")
        raise SystemExit(2)

    all_pairs: Set[tuple[int, int]] = set()
    for f in root_files:
        all_pairs |= extract_pairs_from_file(f)

    if not all_pairs:
        print("[ERROR] Found no (run, subrun) pairs in inputs.")
        raise SystemExit(3)

    total_ext, missing_pairs, by_run = sum_ext_triggers(args.run_db, all_pairs)

    if args.value_only:
        print(total_ext)
        return

    meta = summarize_pairs(all_pairs)
    payload = {
        "generated": datetime.now().isoformat(timespec="seconds"),
        "host": socket.gethostname(),
        "run_db": str(Path(args.run_db).resolve()),
        "n_input_files": len(root_files),
        "n_unique_pairs": meta["n_pairs"],
        "pairs_by_run_counts": meta["by_run_counts"],
        "total_ext_triggers": total_ext,
        "by_run_ext_triggers": by_run,
        "missing_pairs": missing_pairs[:50],  # cap preview
        "notes": "Copy 'total_ext_triggers' into config/data.json as 'ext_triggers' for the EXT sample."
    }

    outp = Path(args.out)
    outp.parent.mkdir(parents=True, exist_ok=True)
    with open(outp, "w") as f:
        json.dump(payload, f, indent=2)

    print(f"[OK] EXT triggers computed: {total_ext}")
    print(f"[OK] Wrote summary JSON -> {outp}")


if __name__ == "__main__":
    main()

