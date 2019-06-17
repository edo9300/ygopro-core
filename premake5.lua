project "ocgcore"
	kind "StaticLib"	
	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
	warnings "Extra"
	optimize "Speed"

	defines "LUA_COMPAT_5_2"

	filter "*DLL"
		kind "SharedLib"

	filter "system:windows"
		links "lua"
		includedirs "../lua"

	filter "action:not vs*"
        buildoptions "-std=c++14"

	filter "system:not windows"
		links "lua5.3-c++"
		buildoptions { "-std=c++14", "-Wno-unused-parameter", "-pedantic" }
		includedirs "/usr/include/lua5.3"

	filter "system:bsd"
		defines "LUA_USE_POSIX"

	filter "system:macosx"
		defines "LUA_USE_MACOSX"
		includedirs "/usr/local/include/lua5.3"

	filter "system:linux"
		defines "LUA_USE_LINUX"