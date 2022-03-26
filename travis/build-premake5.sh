#!/usr/bin/env bash

set -euxo pipefail

if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	./premake5.exe vs2017;
    "$MSBUILD_PATH/msbuild.exe" -m -p:Configuration=$BUILD_CONFIG -p:Platform=Win32 ./build/ocgcore.sln;
else
	./premake5 gmake2;
	make -Cbuild -j2 config=$BUILD_CONFIG;
fi
