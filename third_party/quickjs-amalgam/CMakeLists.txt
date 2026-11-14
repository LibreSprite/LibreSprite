# cmake support for quickjs by LibreSprite contributors

add_library(quickjs quickjs-amalgam.c)
set_target_properties(quickjs PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_compile_definitions(quickjs PUBLIC QJS_BUILD_LIBC)
target_include_directories(quickjs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
