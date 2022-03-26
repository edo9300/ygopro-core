local ocgcore_config=function()
	files { "*.h", "*.cpp" }
	warnings "Extra"
	optimize "Speed"
	cppdialect "C++14"
	defines "LUA_COMPAT_5_2"

	filter "system:not windows"
		buildoptions { "-Wno-unused-parameter", "-pedantic" }

	filter "system:linux"
		defines "LUA_USE_LINUX"

	filter "system:macosx"
		defines "LUA_USE_MACOSX"
end

if not subproject then
newoption {
	trigger = "oldwindows",
	description = "Use some tricks to support up to windows 2000"
}
	workspace "ocgcore"
	location "build"
	language "C++"
	objdir "obj"
	configurations { "Debug", "Release" }
	
	filter "system:windows"
		defines { "WIN32", "_WIN32", "NOMINMAX" }
		platforms {"Win32", "x64"}

	filter "platforms:Win32"
		architecture "x86"

	filter "platforms:x64"
		architecture "x64"

	if _OPTIONS["oldwindows"] then
		filter { "architecture:not *64" , "action:vs*" }
			toolset "v141_xp"

		filter {}
	end
	
	filter "system:not windows"
		includedirs "/usr/local/include"
		libdirs "/usr/local/lib"
	
	filter "action:vs*"
		vectorextensions "SSE2"
		buildoptions "-wd4996"
		defines "_CRT_SECURE_NO_WARNINGS"
	
	filter "action:not vs*"
		buildoptions { "-fno-strict-aliasing", "-Wno-multichar" }
	
	filter "configurations:Debug"
		symbols "On"
		defines "_DEBUG"
		targetdir "bin/debug"
		runtime "Debug"
	
	filter { "configurations:Release" , "action:not vs*" }
		symbols "On"
		defines "NDEBUG"
	
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
	defines "OCGCORE_EXPORT_FUNCTIONS"
	staticruntime "on"
	visibility "Hidden"
	ocgcore_config()
	if _OPTIONS["oldwindows"] then
		filter {}
		files { "./overwrites/overwrites.cpp", "./overwrites/loader.asm" }
		filter "files:**.asm"
			exceptionhandling 'SEH'
		filter {}
	end
	
	filter "system:linux"
		links "lua:static"

	filter "system:macosx or ios"
		links "lua"
