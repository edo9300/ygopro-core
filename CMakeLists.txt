cmake_minimum_required(VERSION 3.12.0)
set(CMAKE_CXX_STANDARD 14)

project(ocgcore)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

option(OCGCORE_MSVC_STATIC "Static CRT on Visual Studio" ON)

find_package(Lua REQUIRED)

add_library(ocgcore_static STATIC)
add_library(ocgcore SHARED)

target_compile_definitions(ocgcore PUBLIC YGOPRO_BUILD_DLL)

target_compile_definitions(ocgcore PUBLIC LUA_COMPAT_5_2)
target_compile_definitions(ocgcore_static PUBLIC LUA_COMPAT_5_2)

target_include_directories(ocgcore PUBLIC ${LUA_INCLUDE_DIR})
target_include_directories(ocgcore_static PUBLIC ${LUA_INCLUDE_DIR})

if(MSVC)
	target_compile_definitions(ocgcore PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_definitions(ocgcore_static PRIVATE _CRT_SECURE_NO_WARNINGS)
	
	find_library(LUA_CXX_LIBRARIES lua-c++)
	message("Lua-C++ found at " ${LUA_CXX_LIBRARIES})
	target_link_libraries(ocgcore PUBLIC ${LUA_CXX_LIBRARIES})
else()
	target_link_libraries(ocgcore PUBLIC ${LUA_LIBRARIES})
endif()

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

target_sources(ocgcore_static PRIVATE ${OCGCORE_SOURCE})
target_sources(ocgcore PRIVATE ${OCGCORE_SOURCE})

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
