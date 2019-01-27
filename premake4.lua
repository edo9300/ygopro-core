project "ocgcore"
	kind "StaticLib"
	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
	flags { "ExtraWarnings", "OptimizeSpeed" }

	defines "LUA_COMPAT_5_2"

	configuration "*DLL"
		kind "SharedLib"

	configuration "windows"
		links "lua"
		includedirs "../lua"

	configuration "not vs*"
		buildoptions "-std=c++14"

	configuration "not windows"
		links "lua5.3-c++"
		buildoptions { "-std=c++14", "-Wno-unused-parameter", "-pedantic" }
		includedirs "/usr/include/lua5.3"

	configuration "macosx"
		defines "LUA_USE_MACOSX"

	configuration "bsd"
		defines "LUA_USE_POSIX"

	configuration "linux"
		defines "LUA_USE_LINUX"