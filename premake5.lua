project "ocgcore"
	kind "StaticLib"	
	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
	warnings "Extra"
	optimize "Speed"
	
	filter "*DLL"
		kind "SharedLib"
	
	filter "system:windows"
		links "lua"
		includedirs "../lua"

	filter "action:not vs*"
        buildoptions "-std=c++14"

	filter "system:not windows"
		links "lua5.3++"
		buildoptions { "-std=c++14", "-Wno-unused-parameter", "-pedantic" }
		includedirs "/usr/include/lua5.3"
