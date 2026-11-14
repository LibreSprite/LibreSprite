cmake_minimum_required(VERSION 4.1)
project(tinyxml2-test)

enable_testing()

find_package(tinyxml2 REQUIRED)

add_executable(xmltest ../xmltest.cpp)
target_link_libraries(xmltest PRIVATE tinyxml2::tinyxml2)

add_test(
    NAME xmltest
    COMMAND xmltest
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_tests_properties(xmltest
                     PROPERTIES
                     PASS_REGULAR_EXPRESSION ", Fail 0"
                     ENVIRONMENT "PATH=$<TARGET_FILE_DIR:tinyxml2::tinyxml2>")
