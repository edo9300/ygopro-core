project "ocgcore"
	kind "StaticLib"

	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
	flags { "ExtraWarnings", "OptimizeSpeed" }
	
	configuration "windows"
		links { "lua" }
		includedirs { ".." }
	configuration "not windows"
		links { "lua5.3" }
		buildoptions { "-std=c++14", "-Wno-unused-parameter", "-pedantic" }
		includedirs { "/usr/include/lua", "/usr/include/lua5.3", "/usr/include/lua/5.3" }
