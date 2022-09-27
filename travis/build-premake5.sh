#!/usr/bin/env bash

set -euxo pipefail

PREMAKE_FLAGS=""
if [[ -n "${TARGET_OS:-""}" ]]; then
	PREMAKE_FLAGS="$PREMAKE_FLAGS --os=macosx"
fi

if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	./premake5.exe vs2019 --oldwindows=true;
	msbuild.exe -p:Configuration=$BUILD_CONFIG -p:Platform=Win32 -t:ocgcoreshared  -verbosity:minimal -p:EchoOff=true ./build/ocgcore.sln;
fi
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    ./premake5 gmake2 $PREMAKE_FLAGS
	make -Cbuild ocgcoreshared -j2 config=$BUILD_CONFIG;
fi
if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    ./premake5 gmake2
	make -Cbuild ocgcoreshared -j3 config=$BUILD_CONFIG;
fi
