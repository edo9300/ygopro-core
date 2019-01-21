project "ocgcore"
	kind "StaticLib"
	configuration "*DLL"
		kind "SharedLib"
	configuration {}

	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
	flags { "ExtraWarnings", "OptimizeSpeed" }

	configuration "windows"
		links "lua"
		includedirs "../lua"

	configuration "not vs*"
		buildoptions "-std=c++14"

	configuration "not windows"
		links "lua5.3-c++"
		buildoptions { "-std=c++14", "-Wno-unused-parameter", "-pedantic" }
		includedirs "/usr/include/lua5.3"