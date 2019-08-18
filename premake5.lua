local ocgcore_config=function()
	files { "**.cc", "**.cpp", "**.c", "**.hh", "**.hpp", "**.h" }
	warnings "Extra"
	optimize "Speed"
	cppdialect "C++14"
	defines "LUA_COMPAT_5_2"

	filter "system:not windows"
		buildoptions { "-Wno-unused-parameter", "-pedantic" }

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
		runtime "Debug"
	
	filter { "configurations:Release" , "action:not vs*" }
		symbols "On"
		defines "NDEBUG"
		buildoptions "-march=native"
	
	filter "configurations:Release"
		optimize "Size"
		targetdir "bin/release"
	
	local function vcpkgStaticTriplet(prj)
		premake.w('<VcpkgTriplet Condition="\'$(Platform)\'==\'Win32\'">x86-windows-static</VcpkgTriplet>')
		premake.w('<VcpkgTriplet Condition="\'$(Platform)\'==\'x64\'">x64-windows-static</VcpkgTriplet>')
	end
	
	require('vstudio')
	
	premake.override(premake.vstudio.vc2010.elements, "globals", function(base, prj)
		local calls = base(prj)
		table.insertafter(calls, premake.vstudio.vc2010.targetPlatformVersionGlobal, vcpkgStaticTriplet)
		return calls
	end)
end

project "ocgcore"
	kind "StaticLib"
	ocgcore_config()

project "ocgcoreshared"
	kind "SharedLib"
	targetname "ocgcore"
	defines "YGOPRO_BUILD_DLL"
	staticruntime "on"
	ocgcore_config()
	
	filter "system:linux or system:bsd"
		links "lua5.3-c++" 
		
	filter "system:macosx"
		links "lua" 
