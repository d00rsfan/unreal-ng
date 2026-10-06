#!/usr/bin/env bash
# Run inside docker/linux/Dockerfile.system-deb (Ubuntu 26.04, x86_64).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
export BUILD_DIR=${BUILD_DIR:-$ROOT/scratch/system-deb-build}
export UNREAL_NO_CONFIGURE=1
cores=$(nproc)
jobs=$((cores / 2))
((jobs > 0)) || jobs=1
export TEST_SHARDS=${TEST_SHARDS:-${UNREAL_JOBS:-$jobs}}

"$ROOT/tools/build/slot.sh" build -- cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_QT_APPS=ON -DTESTS=ON -DBENCHMARKS=OFF \
    -DUNREAL_USE_SYSTEM_LIBS=ON -DUNREAL_BUNDLE_QT=OFF \
    -DUNREAL_HOST_TLS=ON -DTRANTOR_USE_TLS=openssl -DBUILD_C-ARES=OFF \
    -DPACKAGE_SYMBOLS=OFF -DRELEASE_WITH_DEBUG_SYMBOLS=OFF \
    -DUNREAL_PACKAGE_VERSION="${UNREAL_PACKAGE_VERSION:-}" \
    -DUNREAL_PACKAGE_BASENAME=UnrealNG-Suite-Ubuntu-26.04-x86_64
"$ROOT/tools/build/build.sh"
"$ROOT/tools/build/test.sh"
cpack --config "$BUILD_DIR/CPackConfig.cmake" -G DEB
