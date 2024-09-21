project "lua"
	kind "StaticLib"
	pic "On"
	files { "src/*.c", "src/*.h" }
	removefiles { "src/lua.c", "src/luac.c", "src/ltests.h", "src/ltests.c", "src/onelua.c" }
	
	filter "configurations:Release"
		optimize "Speed"	
	filter "configurations:Debug"
		optimize "Off"

	filter { "files:**.c" }
		compileas "C++"
