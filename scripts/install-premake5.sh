#!/usr/bin/env bash

set -euxo pipefail

TRAVIS_OS_NAME=${1:-$TRAVIS_OS_NAME}

if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	curl --retry 5 --connect-timeout 30 --location --remote-header-name --remote-name https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-windows.zip
	unzip -uo premake-5.0.0-beta2-windows.zip
fi
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
	curl --retry 5 --connect-timeout 30 --location --remote-header-name --remote-name https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-linux.tar.gz
	tar xf premake-5.0.0-beta2-linux.tar.gz
fi
if [[ "$TRAVIS_OS_NAME" == "macosx" ]]; then
	curl --retry 5 --connect-timeout 30 --location --remote-header-name --remote-name https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-macosx.tar.gz
	tar xf premake-5.0.0-beta2-macosx.tar.gz
fi
