#!/usr/bin/env python3
"""Aggregate POT from ROOT files.

This script reads a list of ROOT files and sums the ``pot`` branch from the
``nuselection/SubRun`` tree.  The files are processed in parallel using a
``ProcessPoolExecutor``.  The level of parallelism can be controlled with the
``--workers`` CLI option.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import os
import sys
from pathlib import Path
from typing import Iterable, List

import uproot


def _pot_from_files(file_batch: List[str]) -> float:
    """Return the total POT for a batch of ROOT files.

    Each worker opens files using ``uproot.open`` with ``array_cache=None`` to
    avoid caching unnecessary branches. Only the ``pot`` branch is read from the
    ``nuselection/SubRun`` tree.
    """
    total = 0.0
    for file_name in file_batch:
        path = Path(file_name)
        if not path.is_file():
            continue
        try:
            with uproot.open(path, array_cache=None) as root_file:
                try:
                    tree = root_file["nuselection/SubRun"]
                    arr = tree.arrays("pot", library="np")
                    total += float(arr["pot"].sum())
                except KeyError:
                    # Missing tree or branch; ignore this file
                    continue
        except Exception as exc:  # pragma: no cover - defensive
            print(f"Warning: failed to read POT from {path}: {exc}", file=sys.stderr)
    return total


def _chunkify(items: Iterable[str], chunks: int) -> List[List[str]]:
    """Split *items* into ``chunks`` batches."""
    items = list(items)
    if chunks <= 0:
        chunks = 1
    chunk_size = (len(items) + chunks - 1) // chunks
    return [items[i : i + chunk_size] for i in range(0, len(items), chunk_size)]


def aggregate_pot(files: List[str], workers: int) -> float:
    """Aggregate POT from ``files`` using ``workers`` processes."""
    batches = _chunkify(files, workers)
    with concurrent.futures.ProcessPoolExecutor(max_workers=workers) as ex:
        return sum(ex.map(_pot_from_files, batches))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Aggregate POT from ROOT files")
    parser.add_argument("files", nargs="+", help="ROOT files to process")
    parser.add_argument(
        "--workers",
        type=int,
        default=os.cpu_count() or 1,
        help="Number of worker processes (default: number of CPUs)",
    )
    args = parser.parse_args(argv)

    total = aggregate_pot(args.files, args.workers)
    print(total)
    return 0


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    raise SystemExit(main())
