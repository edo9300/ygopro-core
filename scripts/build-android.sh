#!/usr/bin/env bash

set -euxo pipefail

PROCS=""
EXT=""
if [[ "$TRAVIS_OS_NAME" == "macosx" ]]; then
    PROCS=$(sysctl -n hw.ncpu)
else
    PROCS=$(nproc)
fi
if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
    EXT=.cmd
fi

$ANDROID_HOME/ndk-bundle/ndk-build$EXT -j$PROCS
