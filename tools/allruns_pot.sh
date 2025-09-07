#!/bin/bash
# pot_by_run_polarity.sh (FAST SQL version)
# Computes BNB & NuMI totals and NuMI FHC/RHC splits using sqlite3 directly.

: "${LC_ALL:=C}"; export LC_ALL

# --- Environment (minimal) ---
source /cvmfs/uboone.opensciencegrid.org/products/setup_uboone.sh
setup sam_web_client  # harmless; not actually used here
command -v sqlite3 >/dev/null || { echo "sqlite3 not found"; exit 1; }

DBROOT="/exp/uboone/data/uboonebeam/beamdb"
SLIP_DIR="/exp/uboone/app/users/guzowski/slip_stacking"

# Prefer v2 DBs
BNB_DB="$DBROOT/bnb_v2.db";  [[ -f "$BNB_DB"  ]] || BNB_DB="$DBROOT/bnb_v1.db"
NUMI_DB="$DBROOT/numi_v2.db"; [[ -f "$NUMI_DB" ]] || NUMI_DB="$DBROOT/numi_v1.db"
RUN_DB="$DBROOT/run.db"
NUMI_V4_DB="$SLIP_DIR/numi_v4.db"

have_numi_v4=false; [[ -f "$NUMI_V4_DB" ]] && have_numi_v4=true

# Run windows
declare -A RUN_START RUN_END
RUN_START[1]="2015-10-01T00:00:00"; RUN_END[1]="2016-07-31T23:59:59"
RUN_START[2]="2016-10-01T00:00:00"; RUN_END[2]="2017-07-31T23:59:59"
RUN_START[3]="2017-10-01T00:00:00"; RUN_END[3]="2018-07-31T23:59:59"
RUN_START[4]="2018-10-01T00:00:00"; RUN_END[4]="2019-07-31T23:59:59"
RUN_START[5]="2019-10-01T00:00:00"; RUN_END[5]="2020-03-31T23:59:59"

# Pretty print
bar() { printf '%*s\n' 100 '' | tr ' ' '='; }
hdr() { printf "%-7s | %-6s | %14s %14s %14s %14s %14s %14s %14s %14s\n" \
              "Run" "$1" "EXT" "Gate" "Cnt" "TorA" "TorB/Target" "Cnt_wcut" "TorA_wcut" "TorB/Target_wcut"; }
row() { printf "%-7s | %-6s | %14.1f %14.1f %14.1f %14.4g %14.4g %14.1f %14.4g %14.4g\n" "$@"; }

# BNB totals (joins runinfo with bnb_v{1,2}.db)
bnb_totals_sql() {
  local S="$1" E="$2"
  sqlite3 -noheader -list "$RUN_DB" <<SQL
ATTACH '$BNB_DB' AS bnb;
SELECT
  IFNULL(SUM(r.EXTTrig),0.0),
  IFNULL(SUM(r.Gate2Trig),0.0),
  IFNULL(SUM(r.E1DCNT),0.0),
  IFNULL(SUM(r.tor860)*1e12,0.0),
  IFNULL(SUM(r.tor875)*1e12,0.0),
  IFNULL(SUM(b.E1DCNT),0.0),
  IFNULL(SUM(b.tor860)*1e12,0.0),
  IFNULL(SUM(b.tor875)*1e12,0.0)
FROM runinfo AS r
LEFT OUTER JOIN bnb.bnb AS b ON r.run=b.run AND r.subrun=b.subrun
WHERE r.begin_time >= '$S' AND r.end_time <= '$E';
SQL
}

# NuMI totals (joins runinfo with numi_v{1,2}.db)
numi_totals_sql() {
  local S="$1" E="$2"
  sqlite3 -noheader -list "$RUN_DB" <<SQL
ATTACH '$NUMI_DB' AS numi;
SELECT
  IFNULL(SUM(r.EXTTrig),0.0),
  IFNULL(SUM(r.Gate1Trig),0.0),
  IFNULL(SUM(r.EA9CNT),0.0),
  IFNULL(SUM(r.tor101)*1e12,0.0),
  IFNULL(SUM(r.tortgt)*1e12,0.0),
  IFNULL(SUM(n.EA9CNT),0.0),
  IFNULL(SUM(n.tor101)*1e12,0.0),
  IFNULL(SUM(n.tortgt)*1e12,0.0)
FROM runinfo AS r
LEFT OUTER JOIN numi.numi AS n ON r.run=n.run AND r.subrun=n.subrun
WHERE r.begin_time >= '$S' AND r.end_time <= '$E';
SQL
}

# NuMI horns (reads wcut split columns from numi_v4.db directly)
numi_horns_sql() {
  local S="$1" E="$2" which="$3"  # which = FHC or RHC
  local col_sfx; [[ "$which" == "FHC" ]] && col_sfx="fhc" || col_sfx="rhc"
  sqlite3 -noheader -list "$RUN_DB" <<SQL
ATTACH '$NUMI_V4_DB' AS n4;
SELECT
  IFNULL(SUM(n.EA9CNT_${col_sfx}),0.0),
  IFNULL(SUM(n.tor101_${col_sfx})*1e12,0.0),
  IFNULL(SUM(n.tortgt_${col_sfx})*1e12,0.0)
FROM runinfo AS r
LEFT OUTER JOIN n4.numi AS n ON r.run=n.run AND r.subrun=n.subrun
WHERE r.begin_time >= '$S' AND r.end_time <= '$E';
SQL
}

bar
echo "BNB & NuMI POT / Toroid by run period"
echo "FAST SQL path (sqlite3); DBROOT: $DBROOT"
$have_numi_v4 && echo "NuMI FHC/RHC splits from $NUMI_V4_DB" || echo "NuMI FHC/RHC splits disabled (numi_v4.db not found)"
bar

for r in 1 2 3 4 5; do
  S="${RUN_START[$r]}"; E="${RUN_END[$r]}"
  echo "Run $r  (${S} → ${E})"
  echo

  hdr "BNB"
  IFS='|' read -r EXT Gate2 Cnt TorA TorB CntW TorAW TorBW < <(bnb_totals_sql "$S" "$E")
  row "$r" "TOTAL" "$EXT" "$Gate2" "$Cnt" "$TorA" "$TorB" "$CntW" "$TorAW" "$TorBW"

  echo
  hdr "NuMI"
  IFS='|' read -r EXT1 Gate1 CntN Tor101 Tortgt CntNW Tor101W TortgtW < <(numi_totals_sql "$S" "$E")
  row "$r" "TOTAL" "$EXT1" "$Gate1" "$CntN" "$Tor101" "$Tortgt" "$CntNW" "$Tor101W" "$TortgtW"

  if $have_numi_v4; then
    IFS='|' read -r cntF tor101F tortgtF < <(numi_horns_sql "$S" "$E" "FHC")
    row "$r" "FHC"  0 0 0 0 0 "$cntF" "$tor101F" "$tortgtF"
    IFS='|' read -r cntR tor101R tortgtR < <(numi_horns_sql "$S" "$E" "RHC")
    row "$r" "RHC"  0 0 0 0 0 "$cntR" "$tor101R" "$tortgtR"
  fi

  echo
  bar
done

print_legend() {
  cat <<'LEGEND'
Legend / definitions
- Row labels:
  - BNB rows show TOTAL only (no horn split available in official BNB DBs).
  - NuMI rows show TOTAL plus FHC and RHC when numi_v4.db is available.
- Time window: rows integrate runs where r.begin_time >= START and r.end_time <= END for each run period.
- Columns (BNB uses Gate2; NuMI uses Gate1):
  - EXT         : SUM(r.EXTTrig)              — number of EXT triggers (from runinfo).
  - Gate        : SUM(r.Gate2Trig or Gate1Trig) — beam gate triggers (runinfo).
  - Cnt         : SUM(r.E1DCNT) for BNB, SUM(r.EA9CNT) for NuMI (raw counter, runinfo).
  - TorA        : SUM(r.tor860)*1e12 (BNB) or SUM(r.tor101)*1e12 (NuMI) — toroid integral from runinfo.
  - TorB/Target : SUM(r.tor875)*1e12 (BNB) or SUM(r.tortgt)*1e12 (NuMI) — second toroid / target toroid from runinfo.
  - Cnt_wcut    : SUM(b.E1DCNT) (BNB) or SUM(n.EA9CNT) (NuMI) — after beam-quality cuts (from bnb_v{1,2}.db or numi_v{1,2}.db).
  - TorA_wcut   : SUM(b.tor860)*1e12 (BNB) or SUM(n.tor101)*1e12 (NuMI) — after beam-quality cuts.
  - TorB/Target_wcut : SUM(b.tor875)*1e12 (BNB) or SUM(n.tortgt)*1e12 (NuMI) — after beam-quality cuts.
- Units: toroid sums are multiplied by 1e12 in SQL; values are printed as scientific/float (POT-scale).
- NuMI FHC/RHC rows: only the *_wcut columns are populated; other columns are printed as 0 by design.
  Those come from numi_v4.db split fields (EA9CNT_fhc/rhc, tor101_fhc/rhc, tortgt_fhc/rhc) joined on (run,subrun).
- DB sources:
  - runinfo table: timing, triggers, and uncuts toroids/counters.
  - bnb_v{1,2}.db / numi_v{1,2}.db: “_wcut” (beam-quality–cut) totals.
  - numi_v4.db: NuMI horn-polarity–split “_wcut” totals (FHC/RHC).
LEGEND
}

print_legend
