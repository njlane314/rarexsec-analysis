#!/usr/bin/env bash
set -euo pipefail

if ! command -v apptainer >/dev/null 2>&1; then
  echo "apptainer is required to run this script" >&2
  exit 1
fi

IMAGE=${RAREXSEC_IMAGE:-/cvmfs/uboone.opensciencegrid.org/containers/uboone-devel-sl7}

CMD=("$@")
if [ $# -eq 0 ]; then
  CMD=("bash")
fi
CMD_STR=$(printf ' %q' "${CMD[@]}")

BIND="-B $(pwd):/workspace"
for dir in /cvmfs /exp/uboone /uboone /grid /run/user /opt/$USER /home /nashome /data /scratch /web /publicweb /pubhosting /etc/grid-security; do
  if [ -d "$dir" ]; then
    BIND="$BIND -B $dir"
  fi
done
if [ -d /pnfs/uboone ]; then
  BIND="$BIND -B /pnfs/uboone"
fi

exec apptainer exec $BIND "$IMAGE" bash -lc "cd /workspace && source .setup.sh && $CMD_STR"
