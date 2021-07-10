cmake_minimum_required (VERSION 3.4)
project(kamisolver CXX)

add_definitions(
    -DNOMINMAX
    -DWIN32_LEAN_AND_MEAN
    -DVC_EXTRALEAN
    -D_CRT_SECURE_NO_WARNINGS
    -D_SECURE_SCL=0
    -D_SCL_SECURE_NO_WARNINGS
)

option(BUILD_SHARED_LIBS "Build libraries as DLLs" FALSE)

#set(kamisolver_VERSION_MAJOR 0)
#set(kamisolver_VERSION_MINOR 1)
#configure_file("${PROJECT_SOURCE_DIR}/Version.h.in" "${PROJECT_BINARY_DIR}/Version.h")

set(SOURCES
    main.cpp
    Node.cpp
    Node.h
    Puzzle.cpp
    Puzzle.h
    Solver.cpp
    Solver.h
)

add_executable(kamisolver ${SOURCES})
