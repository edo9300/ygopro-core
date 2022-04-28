#!/usr/bin/env bash

set -euxo pipefail

if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	./premake5.exe vs2019 --oldwindows=true;
	msbuild.exe -p:Configuration=$BUILD_CONFIG -p:Platform=Win32 -t:ocgcoreshared  -verbosity:minimal -p:EchoOff=true ./build/ocgcore.sln;
else
	./premake5 gmake2;
	make -Cbuild ocgcoreshared -j2 config=$BUILD_CONFIG;
fi
