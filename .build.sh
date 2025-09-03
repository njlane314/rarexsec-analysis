REPO_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if git -C "${REPO_ROOT}" remote get-url origin >/dev/null 2>&1; then
    git -C "${REPO_ROOT}" pull --ff-only || { return 1; }
fi

BUILD_DIR="${REPO_ROOT}/build"

mkdir -p "${BUILD_DIR}" || { return 1; }
cd "${BUILD_DIR}" || { return 1; }

NUM_CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

cmake "${REPO_ROOT}" || { cd "${REPO_ROOT}"; return 1; }

make -j"${NUM_CORES}" || { cd "${REPO_ROOT}"; return 1; }

ctest || { ctest --rerun-failed --output-on-failure; cd "${REPO_ROOT}"; return 1; }

if [ -n "${LARSOFT_INSTALL:-}" ] && [ -d "${LARSOFT_INSTALL}" ] && [ -n "${LARSOFT_TAR_DEST:-}" ]; then
    tar -czf "${BUILD_DIR}/larsoft.tar.gz" -C "${LARSOFT_INSTALL}" . || { cd "${REPO_ROOT}"; return 1; }
    mv "${BUILD_DIR}/larsoft.tar.gz" "${LARSOFT_TAR_DEST}" || { cd "${REPO_ROOT}"; return 1; }
fi

if [ -n "${ASSET_SOURCE:-}" ] && [ -d "${ASSET_SOURCE}" ] && [ -n "${ASSET_TAR_DEST:-}" ]; then
    tar -czf "${BUILD_DIR}/asset.tar.gz" -C "${ASSET_SOURCE}" . || { cd "${REPO_ROOT}"; return 1; }
    mv "${BUILD_DIR}/asset.tar.gz" "${ASSET_TAR_DEST}" || { cd "${REPO_ROOT}"; return 1; }
fi

cd "${REPO_ROOT}" || { return 0; }
