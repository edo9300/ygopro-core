#!/usr/bin/env bash

set -euxo pipefail

PROCS=""
if [[ "$TRAVIS_OS_NAME" == "macosx" ]]; then
    PROCS=$(sysctl -n hw.ncpu)
else
    PROCS=$(nproc)
fi

$ANDROID_HOME/ndk-bundle/ndk-build -j$PROCS
