# Observable Library
# Copyright (C) 2018 David Capello

include(ExternalProject)
ExternalProject_Add(googlebenchmark-project
  URL https://github.com/google/benchmark/archive/master.zip
  PREFIX "${CMAKE_BINARY_DIR}/googlebenchmark"
  INSTALL_DIR "${CMAKE_BINARY_DIR}/googlebenchmark"
  BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/googlebenchmark/lib/${CMAKE_STATIC_LIBRARY_PREFIX}benchmark${CMAKE_STATIC_LIBRARY_SUFFIX}"
  CMAKE_CACHE_ARGS
    -DBENCHMARK_ENABLE_GTEST_TESTS:BOOL=OFF
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_LIBDIR:PATH=<INSTALL_DIR>/lib)

ExternalProject_Get_Property(googlebenchmark-project install_dir)
set(GOOGLEBENCHMARK_INCLUDE_DIRS ${install_dir}/include)
set(GOOGLEBENCHMARK_LIBRARY ${install_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}benchmark${CMAKE_STATIC_LIBRARY_SUFFIX})

# Create the directory so changing INTERFACE_INCLUDE_DIRECTORIES doesn't fail
file(MAKE_DIRECTORY ${GOOGLEBENCHMARK_INCLUDE_DIRS})

add_library(googlebenchmark STATIC IMPORTED)
set_target_properties(googlebenchmark PROPERTIES
  IMPORTED_LOCATION ${GOOGLEBENCHMARK_LIBRARY}
  INTERFACE_INCLUDE_DIRECTORIES ${GOOGLEBENCHMARK_INCLUDE_DIRS})
add_dependencies(googlebenchmark googlebenchmark-project)

add_executable(obs_benchmarks obs_benchmarks.cpp)
target_link_libraries(obs_benchmarks obs googlebenchmark)
