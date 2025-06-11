#!/bin/bash

REPO_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

BUILD_DIR="${REPO_ROOT}/build"

mkdir -p "${BUILD_DIR}" || { return 1; }
cd "${BUILD_DIR}" || { return 1; }

NUM_CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

cmake "${REPO_ROOT}" || { cd "${REPO_ROOT}"; return 1; }

make -j"${NUM_CORES}" || { cd "${REPO_ROOT}"; return 1; }

cd "${REPO_ROOT}" || { return 0; }