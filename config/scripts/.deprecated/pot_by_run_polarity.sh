#!/bin/bash
# pot_by_run_polarity.sh
# Uses local getDataInfo.py; creates a temp dbdir with the needed files.

# --- Environment ---
source /cvmfs/uboone.opensciencegrid.org/products/setup_uboone.sh
setup python v2_7_15a
setup sam_web_client

# --- Paths ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GETDATA="${SCRIPT_DIR}/getDataInfo.py"        # <-- use local getDataInfo.py

DBROOT="/exp/uboone/data/uboonebeam/beamdb"   # official v1/v2 DBs + run.db
SLIP_DIR="/exp/uboone/app/users/guzowski/slip_stacking"  # has numi_v4.db + pre_1970 file

# Find a numi_v4.db for horn splits
NUMI_V4_DIR=""
for d in "$SCRIPT_DIR" "$SLIP_DIR" "$DBROOT"; do
  [[ -f "$d/numi_v4.db" ]] && { NUMI_V4_DIR="$d"; break; }
done
have_numi_v4=false
[[ -n "$NUMI_V4_DIR" ]] && have_numi_v4=true

# Build a temp dbdir with everything getDataInfo.py expects
TMPDB="$(mktemp -d -t uboone_dbdir.XXXXXX)"
cleanup() { rm -rf "$TMPDB"; }
trap cleanup EXIT

# Symlink required DBs (prefer v2, fallback to v1)
ln -s "$DBROOT/run.db" "$TMPDB/run.db"

for base in bnb numi; do
  if [[ -f "$DBROOT/${base}_v2.db" ]]; then
    ln -s "$DBROOT/${base}_v2.db" "$TMPDB/${base}_v2.db"
  elif [[ -f "$DBROOT/${base}_v1.db" ]]; then
    ln -s "$DBROOT/${base}_v1.db" "$TMPDB/${base}_v1.db"
  fi
done

# Provide pre_1970_runs_subruns.txt (from slip_stacking or empty placeholder)
if [[ -f "$SLIP_DIR/pre_1970_runs_subruns.txt" ]]; then
  ln -s "$SLIP_DIR/pre_1970_runs_subruns.txt" "$TMPDB/pre_1970_runs_subruns.txt"
elif [[ -f "$DBROOT/pre_1970_runs_subruns.txt" ]]; then
  ln -s "$DBROOT/pre_1970_runs_subruns.txt" "$TMPDB/pre_1970_runs_subruns.txt"
else
  : > "$TMPDB/pre_1970_runs_subruns.txt"   # empty file avoids IOError; means no special cases
fi

# --- Pretty-print helpers -----------------------------------------------------
bar() { printf '%*s\n' 100 '' | tr ' ' '='; }
hdr() { printf "%-7s | %-6s | %14s %14s %14s %14s %14s %14s %14s %14s\n" \
              "Run" "$1" "EXT" "Gate" "Cnt" "TorA" "TorB/Target" "Cnt_wcut" "TorA_wcut" "TorB/Target_wcut"; }
row() { printf "%-7s | %-6s | %14.1f %14.1f %14.1f %14.4g %14.4g %14.1f %14.4g %14.4g\n" "$@"; }

last_numeric_line() { awk '/^[[:space:][:digit:].eE+-]+$/ { keep=$0 } END { if (keep!="") print keep; }'; }

parse_fields() {
  awk '{
    $1=$1; n=split($0,a," ");
    if (n<8) { print "0 0 0 0 0 0 0 0"; next; }
    for (i=n-7;i<=n;i++) printf("%s%s", a[i], (i<n ? " " : "\n"));
  }'
}
get8() { local line="${1:-}"; [[ -z "$line" ]] && echo "0 0 0 0 0 0 0 0" || printf "%s\n" "$line" | parse_fields; }

# Expand a 3-number NuMI horn triplet to our 8-field layout (only *_wcut are meaningful)
mk8_from_numi3() {
  local t="$1"
  awk -v s="$t" 'BEGIN{split(s,a," "); printf("0 0 0 0 0 %s %s %s\n", a[1]+0, a[2]+0, a[3]+0)}'
}

# --- Call local getDataInfo.py -----------------------------------------------
run_bnb_totals()  { python2 "$GETDATA" --dbdir "$TMPDB" -v2 --noheader --format-bnb  --where "$1"; }
run_numi_totals() { python2 "$GETDATA" --dbdir "$TMPDB" -v2 --noheader --format-numi --where "$1"; }

numi_v4_horn_triplets() {
  local where="$1" out fhc rhc
  out="$(python2 "$GETDATA" --dbdir "$NUMI_V4_DIR" -v4 --noheader --format-numi --horncurr --where "$where" || true)"
  fhc="$(printf '%s\n' "$out" | awk '/Forward Horn Current:/{print $(NF-2),$(NF-1),$NF; f=1} END{if(!f)print ""}')"
  rhc="$(printf '%s\n' "$out" | awk    '/Reverse Horn Current:/{print $(NF-2),$(NF-1),$NF; f=1} END{if(!f)print ""}')"
  printf "%s;%s\n" "$fhc" "$rhc"
}

# --- Run windows --------------------------------------------------------------
declare -A RUN_START RUN_END
RUN_START[1]="2015-10-01T00:00:00"; RUN_END[1]="2016-07-31T23:59:59"
RUN_START[2]="2016-10-01T00:00:00"; RUN_END[2]="2017-07-31T23:59:59"
RUN_START[3]="2017-10-01T00:00:00"; RUN_END[3]="2018-07-31T23:59:59"
RUN_START[4]="2018-10-01T00:00:00"; RUN_END[4]="2019-07-31T23:59:59"
RUN_START[5]="2019-10-01T00:00:00"; RUN_END[5]="2020-03-31T23:59:59"

# --- Report -------------------------------------------------------------------
bar
echo "BNB & NuMI POT / Toroid by run period"
echo "Using local getDataInfo.py; totals from v2 DBs via temp dbdir: $TMPDB"
if $have_numi_v4; then
  echo "NuMI FHC/RHC splits enabled (found numi_v4.db in ${NUMI_V4_DIR})."
else
  echo "NuMI FHC/RHC splits disabled (no numi_v4.db found)."
fi
bar

for r in 1 2 3 4 5; do
  S="${RUN_START[$r]}"; E="${RUN_END[$r]}"
  echo "Run $r  (${S} → ${E})"
  echo

  hdr "BNB"
  WHERE_ALL="begin_time >= '${S}' AND end_time <= '${E}'"
  BNB_ALL="$(get8 "$(run_bnb_totals  "$WHERE_ALL" | last_numeric_line)")"
  # shellcheck disable=SC2086
  row "$r" "TOTAL" $BNB_ALL

  echo
  hdr "NuMI"
  NUMI_ALL="$(get8 "$(run_numi_totals "$WHERE_ALL" | last_numeric_line)")"
  # shellcheck disable=SC2086
  row "$r" "TOTAL" $NUMI_ALL

  if $have_numi_v4; then
    IFS=';' read -r FHC3 RHC3 <<<"$(numi_v4_horn_triplets "$WHERE_ALL")"
    if [[ -n "${FHC3// }" ]]; then row "$r" "FHC" $(mk8_from_numi3 "$FHC3"); else echo "        (NuMI FHC split not available for this window)"; fi
    if [[ -n "${RHC3// }" ]]; then row "$r" "RHC" $(mk8_from_numi3 "$RHC3"); else echo "        (NuMI RHC split not available for this window)"; fi
  fi

  echo
  bar
done

