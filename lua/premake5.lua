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

	-- Workaround for MinGW i386 targetting crtdll.dll, on that C runtime functions returning double
	-- don't round the values to 64-bit IEEE floats but instead leaves them in the 80-bit float
	-- registers since they use x87 float instructions. This causes an erroneous results in Lua's
	-- math.log as the operation log(x)/log(base) is done on 80-bit doubles instead of 64, specifically,
	-- it affects the log of power of 2s.
	-- Force GCC to emit code that rounds double results to 64-bits
	filter { "action:not vs*", "system:windows", "files:src/lmathlib.c"  }
                buildoptions { "-ffloat-store" }

	filter "configurations:Release"
		optimize "Speed"
	filter "configurations:Debug"
		optimize "Off"

	filter { "files:**.c" }
		compileas "C++"
