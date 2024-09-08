#!/usr/bin/env bash

set -euxo pipefail

TRAVIS_OS_NAME=${1:-$TRAVIS_OS_NAME}
PREMAKE_VERSION=5.0.0-beta2
#ensure the script is always running inside the core's root directory
cd "$(dirname "$0")/.."

mkdir -p tmp
rm -rf tmp/premake

if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	PREMAKE_ARCHIVE=premake-$PREMAKE_VERSION-windows.zip
fi
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
	PREMAKE_ARCHIVE=premake-$PREMAKE_VERSION-linux.tar.gz
fi
if [[ "$TRAVIS_OS_NAME" == "macosx" ]]; then
	PREMAKE_ARCHIVE=premake-$PREMAKE_VERSION-macosx.tar.gz
fi

PREMAKE_ARCHIVE_LOCATION=tmp/"$PREMAKE_ARCHIVE"

if [ ! -f $PREMAKE_ARCHIVE_LOCATION ]; then
	curl --retry 5 --connect-timeout 30 --location --remote-header-name \
		https://github.com/premake/premake-core/releases/download/v$PREMAKE_VERSION/$PREMAKE_ARCHIVE -o "$PREMAKE_ARCHIVE_LOCATION"
fi

mkdir tmp/premake
if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	unzip -uo "$PREMAKE_ARCHIVE_LOCATION" -d tmp/premake
	cp tmp/premake/premake5.exe .
else
	tar xf "$PREMAKE_ARCHIVE_LOCATION" -C tmp/premake
	cp tmp/premake/premake5 .
fi
