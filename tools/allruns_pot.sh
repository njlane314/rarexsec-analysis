#!/bin/bash

# Computes BNB & NuMI totals and NuMI FHC/RHC splits using sqlite3 directly,
# and writes a JSON summary suitable for analysis code.

: "${LC_ALL:=C}"; export LC_ALL

# --- simple arg parsing for JSON path -----------------------------------------
OUTPUT_JSON="pot_summary.json"
if [[ $# -ge 2 && ( "$1" == "-o" || "$1" == "--json" ) ]]; then
  OUTPUT_JSON="$2"; shift 2
elif [[ $# -ge 1 ]]; then
  OUTPUT_JSON="$1"; shift 1
fi

# --- Environment (minimal) ---
source /cvmfs/uboone.opensciencegrid.org/products/setup_uboone.sh
setup sam_web_client  # harmless; not actually used here
command -v sqlite3 >/dev/null || { echo "sqlite3 not found"; exit 1; }

DBROOT="/exp/uboone/data/uboonebeam/beamdb"
SLIP_DIR="/exp/uboone/app/users/guzowski/slip_stacking"

# Prefer v2 DBs
BNB_DB="$DBROOT/bnb_v2.db";   [[ -f "$BNB_DB"  ]] || BNB_DB="$DBROOT/bnb_v1.db"
NUMI_DB="$DBROOT/numi_v2.db"; [[ -f "$NUMI_DB" ]] || NUMI_DB="$DBROOT/numi_v1.db"
RUN_DB="$DBROOT/run.db"
NUMI_V4_DB="$SLIP_DIR/numi_v4.db"

have_numi_v4=false; [[ -f "$NUMI_V4_DB" ]] && have_numi_v4=true
horns_enabled_json=$($have_numi_v4 && echo true || echo false)

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

# --- JSON: header -------------------------------------------------------------
HOSTNAME="$(hostname 2>/dev/null || echo host)"
GEN_TS="$(date -Is)"
{
  echo '{'
  printf '  "generated": "%s",\n' "$GEN_TS"
  printf '  "host": "%s",\n' "$HOSTNAME"
  printf '  "dbroot": "%s",\n' "$DBROOT"
  printf '  "run_db": "%s",\n' "$RUN_DB"
  printf '  "bnb_db": "%s",\n' "$BNB_DB"
  printf '  "numi_db": "%s",\n' "$NUMI_DB"
  printf '  "horns_db": "%s",\n' "${NUMI_V4_DB:-}"
  printf '  "horns_enabled": %s,\n' "$horns_enabled_json"
  echo  '  "unitsTor": "POT",'
  echo  '  "runs": ['
} > "$OUTPUT_JSON"

# --- Human-readable report ----------------------------------------------------
bar
echo "BNB & NuMI POT / Toroid by run period"
echo "FAST SQL path (sqlite3); DBROOT: $DBROOT"
$have_numi_v4 && echo "NuMI FHC/RHC splits from $NUMI_V4_DB" || echo "NuMI FHC/RHC splits disabled (numi_v4.db not found)"
bar

for r in 1 2 3 4 5; do
  S="${RUN_START[$r]}"; E="${RUN_END[$r]}"

  # open run object + beams
  {
    printf '    { "index": %d, "start": "%s", "end": "%s", "beams": {\n' "$r" "$S" "$E"
    echo   '      "BNB": {'
  } >> "$OUTPUT_JSON"

  echo "Run $r  (${S} → ${E})"
  echo

  # ---- BNB ----
  hdr "BNB"
  IFS='|' read -r EXT Gate2 Cnt TorA TorB CntW TorAW TorBW < <(bnb_totals_sql "$S" "$E")
  row "$r" "TOTAL" "$EXT" "$Gate2" "$Cnt" "$TorA" "$TorB" "$CntW" "$TorAW" "$TorBW"

  {
    printf '        "TOTAL": { "EXT": %s, "Gate": %s, "Cnt": %s, "TorA": %s, "TorB_Target": %s, "Cnt_wcut": %s, "TorA_wcut": %s, "TorB_Target_wcut": %s }\n' \
           "$EXT" "$Gate2" "$Cnt" "$TorA" "$TorB" "$CntW" "$TorAW" "$TorBW"
    echo   '      },'
    echo   '      "NuMI": {'
  } >> "$OUTPUT_JSON"

  echo
  # ---- NuMI ----
  hdr "NuMI"
  IFS='|' read -r EXT1 Gate1 CntN Tor101 Tortgt CntNW Tor101W TortgtW < <(numi_totals_sql "$S" "$E")
  row "$r" "TOTAL" "$EXT1" "$Gate1" "$CntN" "$Tor101" "$Tortgt" "$CntNW" "$Tor101W" "$TortgtW"

  if $have_numi_v4; then
    IFS='|' read -r cntF tor101F tortgtF < <(numi_horns_sql "$S" "$E" "FHC")
    IFS='|' read -r cntR tor101R tortgtR < <(numi_horns_sql "$S" "$E" "RHC")
    row "$r" "FHC"  0 0 0 0 0 "$cntF" "$tor101F" "$tortgtF"
    row "$r" "RHC"  0 0 0 0 0 "$cntR" "$tor101R" "$tortgtR"

    {
      printf '        "TOTAL": { "EXT": %s, "Gate": %s, "Cnt": %s, "TorA": %s, "TorB_Target": %s, "Cnt_wcut": %s, "TorA_wcut": %s, "TorB_Target_wcut": %s },\n' \
             "$EXT1" "$Gate1" "$CntN" "$Tor101" "$Tortgt" "$CntNW" "$Tor101W" "$TortgtW"
      printf '        "FHC":   { "EXT": 0, "Gate": 0, "Cnt": 0, "TorA": 0, "TorB_Target": 0, "Cnt_wcut": %s, "TorA_wcut": %s, "TorB_Target_wcut": %s },\n' \
             "$cntF" "$tor101F" "$tortgtF"
      printf '        "RHC":   { "EXT": 0, "Gate": 0, "Cnt": 0, "TorA": 0, "TorB_Target": 0, "Cnt_wcut": %s, "TorA_wcut": %s, "TorB_Target_wcut": %s }\n' \
             "$cntR" "$tor101R" "$tortgtR"
    } >> "$OUTPUT_JSON"
  else
    {
      printf '        "TOTAL": { "EXT": %s, "Gate": %s, "Cnt": %s, "TorA": %s, "TorB_Target": %s, "Cnt_wcut": %s, "TorA_wcut": %s, "TorB_Target_wcut": %s }\n' \
             "$EXT1" "$Gate1" "$CntN" "$Tor101" "$Tortgt" "$CntNW" "$Tor101W" "$TortgtW"
    } >> "$OUTPUT_JSON"
  fi

  # close NuMI + beams + run
  {
    echo   '      }'
    echo   '    }'
    if [[ $r -lt 5 ]]; then
      echo   '    },'
    else
      echo   '    }'
    fi
  } >> "$OUTPUT_JSON"

  echo
  bar
done

# --- Close JSON root and announce path ---------------------------------------
{
  echo '  ]'
  echo '}'
} >> "$OUTPUT_JSON"

echo "JSON written to: $OUTPUT_JSON"

