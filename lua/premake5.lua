newoption {
	trigger = "lua-apicheck",
	description = "build lua with apicheck support"
}
project "lua"
	kind "StaticLib"
	pic "On"
	files { "src/*.c" }
	removefiles {
					"src/lbitlib.c",
					"src/lcorolib.c",
					"src/ldblib.c",
					"src/linit.c",
					"src/loadlib.c",
					"src/loslib.c",
					"src/ltests.c",
					"src/lua.c",
					"src/luac.c",
					"src/lutf8lib.c",
					"src/onelua.c"
				}
	
	includedirs { "./" }
	forceincludes { "luaconf-customize.h" }
	if _OPTIONS["lua-apicheck"] then
		defines "LUA_EPRO_APICHECK"
	end
	
	filter "configurations:Release"
		optimize "Speed"	
	filter "configurations:Debug"
		optimize "Off"

	filter { "files:**.c" }
		compileas "C++"
