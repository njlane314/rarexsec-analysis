#!/usr/bin/env bash
set -euo pipefail

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this script" >&2
  exit 1
fi

IMAGE=${RAREXSEC_IMAGE:-ghcr.io/rarexsec/analysis:latest}
UID_GID="$(id -u):$(id -g)"
CMD=("$@")
if [ $# -eq 0 ]; then
  CMD=("bash")
fi

exec docker run --rm -it \
    -u "${UID_GID}" \
    -v "$(pwd)":/workspace \
    -w /workspace \
    "${IMAGE}" "${CMD[@]}"
