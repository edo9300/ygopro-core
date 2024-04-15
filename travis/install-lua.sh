#!/usr/bin/env bash

set -euxo pipefail

curl --retry 2 --connect-timeout 30 --location --remote-header-name --remote-name https://www.lua.org/ftp/lua-5.3.6.tar.gz || \
curl --retry 2 --connect-timeout 30 --location --remote-header-name --remote-name https://www.tecgraf.puc-rio.br/lua/mirror/ftp/lua-5.3.6.tar.gz
mkdir -p lua
tar xf lua-5.3.6.tar.gz --strip-components=1 -C lua
cd lua
	cat <<EOT >> src/luaconf.h

#ifdef LUA_USE_WINDOWS
#undef LUA_USE_WINDOWS
#undef LUA_DL_DLL
#undef LUA_USE_C89
#endif

EOT
if [[ -n ${LUA_APICHECK:-""} ]]; then
	cat <<EOT >> src/luaconf.h

#ifndef luaconf_h_ocgcore_api_check
#define luaconf_h_ocgcore_api_check
void ocgcore_lua_api_check(void* L, const char* error_message);
#define luai_apicheck(l,e)	do { if(!e) ocgcore_lua_api_check(l,"Lua api check failed: " #e); } while(0)


#endif
EOT
fi
