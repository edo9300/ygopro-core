local ocgcore_config=function()
	files { "*.h", "*.hpp", "*.cpp", "RNG/*.hpp", "RNG/*.cpp" }
	warnings "Extra"
	optimize "Speed"
	cppdialect "C++17"
	rtti "Off"

	filter "action:not vs*"
		buildoptions { "-Wno-unused-parameter", "-pedantic" }
	filter "system:linux"
		linkoptions { "-Wl,--no-undefined" }
	filter { "system:macosx", "files:processor_visit.cpp" }
		buildoptions { "-fno-exceptions" }
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
	startproject "ocgcoreshared"

	filter "system:windows"
		defines { "WIN32", "_WIN32", "NOMINMAX" }
		platforms {"Win32", "x64", "arm", "arm64"}

	filter "platforms:Win32"
		architecture "x86"

	filter "platforms:x64"
		architecture "x64"

	filter "platforms:arm64"
		architecture "ARM64"

	filter "platforms:arm"
		architecture "ARM"

	filter { "action:vs*", "platforms:Win32 or x64" }
		vectorextensions "SSE2"
		if _OPTIONS["oldwindows"] then
			toolset "v141_xp"
		end

	filter "action:vs*"
		flags "MultiProcessorCompile"

	filter "configurations:Debug"
		defines "_DEBUG"
		targetdir "bin/debug"
		runtime "Debug"

	filter "configurations:Release"
		optimize "Size"
		targetdir "bin/release"
		defines "NDEBUG"

	local function set_target_dir(target,arch)
		filter { "system:windows", "configurations:" .. target, "architecture:" .. arch }
			targetdir("bin/" .. arch .. "/" .. target)
	end

	set_target_dir("debug","x64")
	set_target_dir("debug","arm")
	set_target_dir("debug","arm64")

	set_target_dir("release","x64")
	set_target_dir("release","arm")
	set_target_dir("release","arm64")

	filter { "action:not vs*", "system:windows" }
		buildoptions { "-static-libgcc", "-static-libstdc++", "-static" }
		linkoptions { "-static-libgcc", "-static-libstdc++", "-static" }
		defines { "UNICODE", "_UNICODE" }

	filter { "system:linux" }
		linkoptions { "-static-libgcc", "-static-libstdc++" }

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
end

project "ocgcore"
	kind "StaticLib"
	ocgcore_config()

project "ocgcoreshared"
	kind "SharedLib"
	flags "NoImportLib"
	filter "configurations:Release"
		flags "LinkTimeOptimization"
	filter {}
	targetname "ocgcore"
	defines "OCGCORE_EXPORT_FUNCTIONS"
	staticruntime "on"
	visibility "Hidden"
	ocgcore_config()
