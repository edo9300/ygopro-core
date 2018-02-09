project "ocgcore"
	kind "StaticLib"

	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
    links { "lua" }
	buildoptions { "-Wall", "-Wextra", "-pedantic" }
	
	configuration "windows"
		includedirs { ".." }
	configuration "not windows"
		buildoptions { "-std=c++11", "-Wno-unused-parameter" }
		includedirs { "/usr/include/lua", "/usr/include/lua5.3", "/usr/include/lua/5.3" }
