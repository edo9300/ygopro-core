cmake_minimum_required(VERSION 3.12.0)
set(CMAKE_CXX_STANDARD 14)

project(ocgcore)

find_package(Lua REQUIRED)

add_compile_definitions(ocgcore PUBLIC LUA_COMPAT_5_2)
include_directories(${LUA_INCLUDE_DIR})

add_library(ocgcore_static STATIC)
add_library(ocgcore SHARED)
target_compile_definitions(ocgcore PUBLIC YGOPRO_BUILD_DLL)
target_link_libraries(ocgcore PUBLIC ${LUA_LIBRARIES})

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
