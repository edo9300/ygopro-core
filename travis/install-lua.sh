#!/usr/bin/env bash

set -euxo pipefail

TRAVIS_OS_NAME=${1:-$TRAVIS_OS_NAME}
TARGET_OS=${TARGET_OS:-$TRAVIS_OS_NAME}
CXX=${CXX:-g++}
MAKE=${MAKE:-sudo make}

if [[ "$TARGET_OS" == "windows" ]]; then
	mkdir -p "$VCPKG_ROOT"
	cd "$VCPKG_ROOT"
	curl --retry 5 --connect-timeout 30 --location --remote-header-name --output installed.zip "$VCPKG_CACHE_ZIP_URL"
	unzip -uo installed.zip > /dev/null
	mkdir ports
	mkdir triplets
	mkdir triplets/community
	./vcpkg.exe integrate install
else
	cd /tmp
	curl --retry 5 --connect-timeout 30 --location --remote-header-name --remote-name https://www.lua.org/ftp/lua-5.3.5.tar.gz
	tar xf lua-5.3.5.tar.gz
	cd lua-5.3.5
	if [[ "$TARGET_OS" == "osx" ]]; then
	  make -j3 macosx CC=$CXX MYCFLAGS="${CFLAGS:-""}" MYLDFLAGS="${LDFLAGS:-""}" AR="${AR:-"ar rcu"}" RANLIB="${RANLIB:-"ranlib"}"
	else
	  make -j2 posix CC=$CXX MYCFLAGS="${CFLAGS:-"-fPIC"}" MYLDFLAGS="${LDFLAGS:-""}" AR="${AR:-"ar rcu"}" RANLIB="${RANLIB:-"ranlib"}"
	fi
	$MAKE install
fi
