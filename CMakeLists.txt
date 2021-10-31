cmake_minimum_required(VERSION 3.12.0)
set(CMAKE_CXX_STANDARD 14)

project(ocgcore)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

option(OCGCORE_MSVC_STATIC "Static CRT on Visual Studio" ON)
option(OCGCORE_OLD_WINDOWS "Tricks to support up to windows 2000" OFF)

find_package(Lua REQUIRED)

add_library(ocgcore_static STATIC)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
add_library(ocgcore SHARED)

target_compile_definitions(ocgcore PUBLIC OCGCORE_EXPORT_FUNCTIONS)
target_compile_definitions(ocgcore PUBLIC LUA_COMPAT_5_2)
target_compile_definitions(ocgcore_static PUBLIC LUA_COMPAT_5_2)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(ocgcore PRIVATE _DEBUG)
	target_compile_definitions(ocgcore_static PRIVATE _DEBUG)
endif()
target_include_directories(ocgcore PUBLIC ${LUA_INCLUDE_DIR})
target_include_directories(ocgcore_static PUBLIC ${LUA_INCLUDE_DIR})

find_library(LUA_CXX_LIBRARIES lua-c++)
if(LUA_CXX_LIBRARIES)
	message("Lua-C++ found at " ${LUA_CXX_LIBRARIES})
	target_link_libraries(ocgcore PUBLIC ${LUA_CXX_LIBRARIES})
else()
	message("Lua-C++ not found, using regular lib.")
	target_link_libraries(ocgcore PUBLIC ${LUA_LIBRARIES})
endif()

if(MSVC)
	target_compile_definitions(ocgcore PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_definitions(ocgcore_static PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set(CLANG_OR_GNU $<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:AppleClang>>)
set(OCGCORE_COMPILE_OPTIONS
	# Clang and GNU compilers
	$<${CLANG_OR_GNU}:-Wall -Wextra -pedantic>
	$<$<AND:$<CONFIG:Debug>,${CLANG_OR_GNU}>:-ggdb3 -Og>
	$<$<AND:$<CONFIG:Release>,${CLANG_OR_GNU}>:-O3>
	$<$<AND:$<CONFIG:RelWithDebInfo>,${CLANG_OR_GNU}>:-ggdb3 -O3>
	# Microsoft compiler
	$<$<CXX_COMPILER_ID:MSVC>:/W4>)
target_compile_options(ocgcore PRIVATE ${OCGCORE_COMPILE_OPTIONS})
target_compile_options(ocgcore_static PRIVATE ${OCGCORE_COMPILE_OPTIONS})

set(OCGCORE_SOURCE
	card.cpp
	card.h
	common.h
	duel.cpp
	duel.h
	effect.cpp
	effect.h
	effectset.h
	field.cpp
	field.h
	group.cpp
	group.h
	interpreter.cpp
	interpreter.h
	libcard.cpp
	libdebug.cpp
	libduel.cpp
	libeffect.cpp
	libgroup.cpp
	ocgapi.cpp
	ocgapi.h
	operations.cpp
	playerop.cpp
	processor.cpp
	progressivebuffer.h
	scriptlib.cpp
	scriptlib.h)

if(OCGCORE_OLD_WINDOWS AND MSVC)
	set(OLD_WINDOWS_SOURCES
		overwrites/loader.asm
		overwrites/overwrites.cpp)
endif()

target_sources(ocgcore_static PRIVATE ${OCGCORE_SOURCE})
target_sources(ocgcore PRIVATE ${OCGCORE_SOURCE} ${OLD_WINDOWS_SOURCES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ocgcore)
if(OCGCORE_MSVC_STATIC AND MSVC)
	set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
	foreach(CompilerFlag ${CompilerFlags})
  		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
	# CMake 3.15
	# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if(OCGCORE_OLD_WINDOWS AND MSVC)
ENABLE_LANGUAGE(ASM_MASM)
set_source_files_properties(
    overwrites/loader.asm
    PROPERTIES
    COMPILE_FLAGS "/safeseh"
)

endif()
