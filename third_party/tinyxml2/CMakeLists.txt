cmake_minimum_required(VERSION 4.1)
project(tinyxml2 VERSION 9.0.0)

include(CTest)
option(tinyxml2_BUILD_TESTING "Build tests for tinyxml2" "${BUILD_TESTING}")

##
## Honor tinyxml2_SHARED_LIBS to match install interface
##

if (DEFINED tinyxml2_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${tinyxml2_SHARED_LIBS}")
endif ()

##
## Main library build
##

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

add_library(tinyxml2 tinyxml2.cpp tinyxml2.h)
add_library(tinyxml2::tinyxml2 ALIAS tinyxml2)

# Uncomment the following line to require C++11 (or greater) to use tinyxml2
# target_compile_features(tinyxml2 PUBLIC cxx_std_11)
target_include_directories(tinyxml2 PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_compile_definitions(
    tinyxml2
    PUBLIC $<$<CONFIG:Debug>:TINYXML2_DEBUG>
    INTERFACE $<$<BOOL:${BUILD_SHARED_LIBS}>:TINYXML2_IMPORT>
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

set_target_properties(
    tinyxml2
    PROPERTIES
    DEFINE_SYMBOL "TINYXML2_EXPORT"
    VERSION "${tinyxml2_VERSION}"
    SOVERSION "${tinyxml2_VERSION_MAJOR}"
)

if (tinyxml2_BUILD_TESTING)
    add_executable(xmltest xmltest.cpp)
    target_link_libraries(xmltest PRIVATE tinyxml2::tinyxml2)

    add_test(
        NAME xmltest
        COMMAND xmltest
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    set_tests_properties(xmltest PROPERTIES PASS_REGULAR_EXPRESSION ", Fail 0")
endif ()

##
## Installation
##

## Standard modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

## Custom locations
set(tinyxml2_INSTALL_PKGCONFIGDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    CACHE PATH "Directory for pkgconfig files")

set(tinyxml2_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/tinyxml2"
    CACHE STRING "Path to tinyxml2 CMake files")

## CMake targets and export scripts

install(
    TARGETS tinyxml2 EXPORT tinyxml2-targets
    RUNTIME COMPONENT tinyxml2_runtime
    LIBRARY COMPONENT tinyxml2_runtime
    NAMELINK_COMPONENT tinyxml2_development
    ARCHIVE COMPONENT tinyxml2_development
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Type-specific targets

if (BUILD_SHARED_LIBS)
    set(type shared)
else ()
    set(type static)
endif ()

install(
    EXPORT tinyxml2-targets
    DESTINATION "${tinyxml2_INSTALL_CMAKEDIR}"
    NAMESPACE tinyxml2::
    FILE tinyxml2-${type}-targets.cmake
    COMPONENT tinyxml2_development
)

# Auto-generated version compatibility file
write_basic_package_version_file(
    tinyxml2-config-version.cmake
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tinyxml2-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/tinyxml2-config-version.cmake"
    DESTINATION "${tinyxml2_INSTALL_CMAKEDIR}"
    COMPONENT tinyxml2_development
)

## Headers

install(
    FILES tinyxml2.h
    TYPE INCLUDE
    COMPONENT tinyxml2_development
)

## pkg-config

configure_file(cmake/tinyxml2.pc.in tinyxml2.pc.gen @ONLY)
file(GENERATE OUTPUT tinyxml2.pc INPUT "${CMAKE_CURRENT_BINARY_DIR}/tinyxml2.pc.gen")
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/tinyxml2.pc"
    DESTINATION "${tinyxml2_INSTALL_PKGCONFIGDIR}"
    COMPONENT tinyxml2_development
)
