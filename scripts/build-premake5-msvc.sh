#!/usr/bin/env bash
set -euxo pipefail

VISUAL_STUDIO_VERSION=${1:-$VISUAL_STUDIO_VERSION}
PREMAKE_FLAGS=${2:-$PREMAKE_FLAGS}
VISUAL_STUDIO_ARCH="${VISUAL_STUDIO_ARCH:-Win32}"

if [[ -z "${VISUAL_STUDIO_VERSION}" ]]; then
	VISUAL_STUDIO_VERSION=vs2019
fi

echo ./premake5.exe $VISUAL_STUDIO_VERSION "$PREMAKE_FLAGS"

./premake5.exe $VISUAL_STUDIO_VERSION "$PREMAKE_FLAGS"
msbuild.exe -p:Configuration=$BUILD_CONFIG -p:Platform=$VISUAL_STUDIO_ARCH -t:ocgcoreshared -verbosity:minimal -p:EchoOff=true ./build/ocgcore.sln
