cmake_minimum_required(VERSION 2.8)

project(kamisolver)

set(DYN_LIBS_ONLY ON)
add_subdirectory(dyn)
include_directories(dyn/src)

add_executable(kamisolver main.c)
target_link_libraries(kamisolver dyn)
