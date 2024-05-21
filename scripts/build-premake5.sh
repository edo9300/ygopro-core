#!/usr/bin/env bash

set -euxo pipefail

PREMAKE_FLAGS=${1:-${PREMAKE_FLAGS:-""}}

if [[ -n "${TARGET_OS:-""}" ]]; then
	PREMAKE_FLAGS="$PREMAKE_FLAGS --os=$TARGET_OS"
fi

PROCS=""
if [[ "$TRAVIS_OS_NAME" == "macosx" ]]; then
    PROCS=$(sysctl -n hw.ncpu)
else
    PROCS=$(nproc)
fi

./premake5 gmake2 $PREMAKE_FLAGS
make -Cbuild ocgcoreshared -j$PROCS config=$BUILD_CONFIG
