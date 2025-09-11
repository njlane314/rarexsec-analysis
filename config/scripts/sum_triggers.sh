#!/usr/bin/env bash
# Sum EXT hardware triggers for the (run,subrun) pairs in a CSV (no header).
# Usage: ./sum_ext_triggers.sh [/path/to/run.db] [/path/to/pairs.csv]
# Defaults:
#   run.db  -> /exp/uboone/data/uboonebeam/beamdb/run.db
#   pairs   -> /tmp/numi_fhc_run1_pairs.csv

set -euo pipefail

RUN_DB="${1:-/exp/uboone/data/uboonebeam/beamdb/run.db}"
PAIRS_CSV="${2:-/tmp/numi_fhc_run1_pairs.csv}"

# Basic checks
command -v sqlite3 >/dev/null || { echo "ERROR: sqlite3 not found in PATH." >&2; exit 1; }
[[ -f "$RUN_DB" ]]   || { echo "ERROR: run DB not found: $RUN_DB" >&2; exit 1; }
[[ -f "$PAIRS_CSV" ]]|| { echo "ERROR: pairs CSV not found: $PAIRS_CSV" >&2; exit 1; }

sqlite3 "$RUN_DB" <<SQL
.mode csv
.headers off
CREATE TEMP TABLE pairs(run INTEGER, subrun INTEGER);
.import "$PAIRS_CSV" pairs
SELECT IFNULL(SUM(r.EXTTrig),0) AS total_ext_triggers
FROM runinfo r
JOIN pairs p ON r.run=p.run AND r.subrun=p.subrun;
SQL

