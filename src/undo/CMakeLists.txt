# Undo Library
# Copyright (C) 2015 David Capello

cmake_minimum_required(VERSION 4.1)

project(undo CXX)
option(UNDO_TESTS "Compile undo tests" on)

include_directories(.)

if(UNIX)
  add_definitions(-std=c++11)
endif()

add_library(undo undo_history.cpp)

if(UNDO_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
