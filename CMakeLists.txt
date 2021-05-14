cmake_minimum_required(VERSION 3.5)

option(BUILD_TEST "Build tests" ON)

include (CheckFunctionExists)
include (CheckIncludeFiles)
include (CheckSymbolExists)

project (libfibconcurrent)

include_directories (include)

set(REQURED_LIBS)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
  add_definitions(-DDEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

add_subdirectory (src)

if (BUILD_TEST)
    include(CTest)
    add_subdirectory(test)
endif() # END TEST
