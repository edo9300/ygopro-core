local ocgcore_config=function()
	files { "*.h", "*.hpp", "*.cpp", "RNG/*.hpp", "RNG/*.cpp" }
	warnings "Extra"
	optimize "Speed"
	cppdialect "C++17"
	rtti "Off"

	filter "action:not vs*"
		buildoptions { "-Wno-unused-parameter", "-pedantic" }
	if os.istarget("macosx") then
		filter { "files:processor_visit.cpp" }
			buildoptions { "-fno-exceptions" }
	end
	filter {}
		include "lua"
		links { "lua" }
		includedirs { "lua/src" }
end

if not subproject then
	newoption {
		trigger = "oldwindows",
		description = "Use the v141_xp toolset to support windows XP sp3"
	}
	workspace "ocgcore"
	location "build"
	language "C++"
	objdir "obj"
	configurations { "Debug", "Release" }
	symbols "On"
	staticruntime "on"

	if _OPTIONS["oldwindows"] then
		toolset "v141_xp"
	end

	filter "system:windows"
		defines { "WIN32", "_WIN32", "NOMINMAX" }
		platforms {"Win32", "x64"}

	filter "platforms:Win32"
		architecture "x86"

	filter "platforms:x64"
		architecture "x64"

	filter "action:vs*"
		flags "MultiProcessorCompile"
		vectorextensions "SSE2"

	filter "configurations:Debug"
		defines "_DEBUG"
		targetdir "bin/debug"
		runtime "Debug"

	filter { "action:vs*", "configurations:Debug", "architecture:*64" }
		targetdir "bin/x64/debug"

	filter "configurations:Release"
		optimize "Size"
		targetdir "bin/release"
		defines "NDEBUG"

	filter { "action:vs*", "configurations:Release", "architecture:*64" }
		targetdir "bin/x64/release"

	filter { "action:not vs*", "system:windows" }
		buildoptions { "-static-libgcc", "-static-libstdc++", "-static", "-lpthread" }
		linkoptions { "-mthreads", "-municode", "-static-libgcc", "-static-libstdc++", "-static", "-lpthread" }
		defines { "UNICODE", "_UNICODE" }

	local function disableWinXPWarnings(prj)
		premake.w('<XPDeprecationWarning>false</XPDeprecationWarning>')
	end

	local function vcpkgStaticTriplet202006(prj)
		premake.w('<VcpkgEnabled>false</VcpkgEnabled>')
		premake.w('<VcpkgUseStatic>false</VcpkgUseStatic>')
		premake.w('<VcpkgAutoLink>false</VcpkgAutoLink>')
	end

	require('vstudio')

	premake.override(premake.vstudio.vc2010.elements, "globals", function(base, prj)
		local calls = base(prj)
		table.insertafter(calls, premake.vstudio.vc2010.targetPlatformVersionGlobal, disableWinXPWarnings)
		table.insertafter(calls, premake.vstudio.vc2010.globals, vcpkgStaticTriplet202006)
		return calls
	end)
	startproject "ocgcoreshared"
end

project "ocgcore"
	kind "StaticLib"
	ocgcore_config()

project "ocgcoreshared"
	kind "SharedLib"
	flags "NoImportLib"
	targetname "ocgcore"
	defines "OCGCORE_EXPORT_FUNCTIONS"
	staticruntime "on"
	visibility "Hidden"
	ocgcore_config()
