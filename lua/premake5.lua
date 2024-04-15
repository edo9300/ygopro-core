project "lua"
	kind "StaticLib"
	pic "On"
	files { "src/*.c", "src/*.h" }
	excludes { "src/lua.c", "src/luac.c" }

	filter { "files:**.c" }
		compileas "C++"
