local ocgcore_config=function()
	files { "**.h", "**.hpp", "**.cpp" }
	warnings "Extra"
	optimize "Speed"
	cppdialect "C++17"

	filter "action:not vs*"
		buildoptions { "-Wno-unused-parameter", "-pedantic" }
	if os.istarget("macosx") then
		filter { "files:processor_visit.cpp" }
			buildoptions { "-fno-exceptions" }
	end
end

if not subproject then
	newoption {
		trigger = "oldwindows",
		description = "Use the v140_xp or v141_xp toolset to support windows XP sp3"
	}
	newoption {
		trigger = "lua-path",
		description = "Path where the lua library has been installed"
	}
	workspace "ocgcore"
	location "build"
	language "C++"
	objdir "obj"
	configurations { "Debug", "Release" }
	symbols "On"

	filter "system:windows"
		defines { "WIN32", "_WIN32", "NOMINMAX" }
		platforms {"Win32", "x64"}

	filter "platforms:Win32"
		architecture "x86"

	filter "platforms:x64"
		architecture "x64"

	if _OPTIONS["oldwindows"] then
		filter {}
			toolset "v141_xp"
	end

	filter "action:vs*"
		flags "MultiProcessorCompile"
		vectorextensions "SSE2"

	filter "action:not vs*"
		buildoptions "-fno-strict-aliasing"
		if _OPTIONS["lua-path"] then
			includedirs{ _OPTIONS["lua-path"] .. "/include" }
			libdirs{ _OPTIONS["lua-path"] .. "/lib" }
		else
			includedirs "/usr/local/include"
			libdirs "/usr/local/lib"
		end

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

	local function vcpkgStaticTriplet(prj)
		premake.w('<VcpkgTriplet Condition="\'$(Platform)\'==\'Win32\'">x86-windows-static</VcpkgTriplet>')
		premake.w('<VcpkgTriplet Condition="\'$(Platform)\'==\'x64\'">x64-windows-static</VcpkgTriplet>')
	end

	local function disableWinXPWarnings(prj)
		premake.w('<XPDeprecationWarning>false</XPDeprecationWarning>')
	end

	local function vcpkgStaticTriplet202006(prj)
		premake.w('<VcpkgEnabled>true</VcpkgEnabled>')
		premake.w('<VcpkgUseStatic>true</VcpkgUseStatic>')
		premake.w('<VcpkgAutoLink>true</VcpkgAutoLink>')
	end

	require('vstudio')

	premake.override(premake.vstudio.vc2010.elements, "globals", function(base, prj)
		local calls = base(prj)
		table.insertafter(calls, premake.vstudio.vc2010.targetPlatformVersionGlobal, vcpkgStaticTriplet)
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
	targetname "ocgcore"
	defines "OCGCORE_EXPORT_FUNCTIONS"
	staticruntime "on"
	visibility "Hidden"
	ocgcore_config()

	filter "action:not vs*"
		links "lua"
