local ocgcore_config=function()
	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
	warnings "Extra"
	optimize "Speed"

	defines "LUA_COMPAT_5_2"

	filter "action:not vs*"
        buildoptions "-std=c++14"

	filter "system:not windows"
		buildoptions { "-std=c++14", "-Wno-unused-parameter", "-pedantic" }

	filter "system:bsd"
		defines "LUA_USE_POSIX"

	filter "system:macosx"
		defines "LUA_USE_MACOSX"

	filter "system:linux"
		defines "LUA_USE_LINUX"
		includedirs "/usr/include/lua5.3"
end

if not subproject then
	workspace "ocgcore"
	location "build"
	language "C++"
	objdir "obj"	
	configurations { "Debug", "Release" }
	
	filter "system:windows"
		defines { "WIN32", "_WIN32", "NOMINMAX" }
	
	filter "system:bsd"
		includedirs "/usr/local/include"
		libdirs "/usr/local/lib"
	
	filter "system:macosx"
		toolset "clang"
		includedirs "/usr/local/include"
		libdirs "/usr/local/lib"
	
	filter "action:vs*"
		vectorextensions "SSE2"
		buildoptions "-wd4996"
		defines "_CRT_SECURE_NO_WARNINGS"
	
	filter "action:not vs*"
		buildoptions { "-fno-strict-aliasing", "-Wno-multichar" }
	
	filter { "action:not vs*", "system:windows" }
		buildoptions { "-static-libgcc" }
	
	filter "configurations:Debug"
		symbols "On"
		defines "_DEBUG"
		targetdir "bin/debug"
	
	filter { "configurations:Release" , "action:not vs*" }
		symbols "On"
		defines "NDEBUG"
		buildoptions "-march=native"
	
	filter "configurations:Release"
		optimize "Size"
		targetdir "bin/release"
end

project "ocgcore"
	kind "StaticLib"
	ocgcore_config()

project "ocgcoreshared"
	kind "SharedLib"
	targetname "ocgcore"
	ocgcore_config()
	
	filter "system:linux or system:bsd"
		links "lua5.3-c++" 
		
	filter "system:macosx"
		links "lua" 
