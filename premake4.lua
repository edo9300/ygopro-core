project "ocgcore"
	kind "StaticLib"
	filter "*DLL"
		kind "SharedLib"
	filter {}
	
	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
	warnings "Extra"
	optimize "Speed"
	
	configuration "windows"
		links { "lua" }
		includedirs { "../lua" }
	configuration "not vs*"
        buildoptions { "-std=c++14" }
	configuration "not windows"
		links { "lua5.3++" }
		buildoptions { "-std=c++14", "-Wno-unused-parameter", "-pedantic" }
		includedirs { "/usr/include/lua5.3" }
