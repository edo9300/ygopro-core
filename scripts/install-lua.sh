#!/usr/bin/env bash

set -euxo pipefail

LUA_VERSION=5.3.6
LUA_ARCHIVE=tmp/lua-$LUA_VERSION.tar.gz
#ensure the script is always running inside the core's root directory
cd "$(dirname "$0")/.."

mkdir -p tmp
rm -rf lua/src

if [ ! -f $LUA_ARCHIVE ]; then
	curl --retry 2 --connect-timeout 30 --location https://github.com/lua/lua/archive/refs/tags/v$LUA_VERSION.tar.gz -o $LUA_ARCHIVE
fi

mkdir -p lua/src
tar xf $LUA_ARCHIVE --strip-components=1 -C lua/src
