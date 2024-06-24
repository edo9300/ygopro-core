#!/usr/bin/env bash

set -euxo pipefail

export LUA_VERSION=5.3.6
curl --retry 2 --connect-timeout 30 --location https://github.com/lua/lua/archive/refs/tags/v$LUA_VERSION.tar.gz -o lua-$LUA_VERSION.tar.gz
rm -rf lua/src
mkdir -p lua/src
tar xf lua-$LUA_VERSION.tar.gz --strip-components=1 -C lua/src
cd lua
	cat <<EOT >> src/luaconf.h

#ifdef LUA_USE_WINDOWS
#undef LUA_USE_WINDOWS
#undef LUA_DL_DLL
#undef LUA_USE_C89
#endif

EOT
if [[ "${LUA_APICHECK:-0}" == "1" ]]; then
	cat <<EOT >> src/luaconf.h

#ifndef luaconf_h_ocgcore_api_check
#define luaconf_h_ocgcore_api_check
void ocgcore_lua_api_check(void* L, const char* error_message);
#define luai_apicheck(l,e)	do { if(!e) ocgcore_lua_api_check(l,"Lua api check failed: " #e); } while(0)


#endif
EOT
fi
