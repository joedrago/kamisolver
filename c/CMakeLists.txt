cmake_minimum_required(VERSION 2.8)

project(kamisolver)

set(DYN_LIBS_ONLY ON)
add_subdirectory(dyn)
include_directories(dyn/src)

if(UNIX)
    add_definitions(-g)
endif()

if(WIN32)
    add_definitions(/wd4996)
endif()
add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)

add_executable(kamisolver main.c)
target_link_libraries(kamisolver dyn)
