# Observable Library
# Copyright (C) 2016-2021 David Capello

cmake_minimum_required(VERSION 3.2)

project(observable CXX)
option(OBSERVABLE_TESTS "Compile observable tests" ON)
option(OBSERVABLE_BENCHMARKS "Compile observable benchmarks" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX AND NOT APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
endif()

add_library(obs obs/connection.cpp)
target_include_directories(obs PUBLIC .)

if(OBSERVABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(OBSERVABLE_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
