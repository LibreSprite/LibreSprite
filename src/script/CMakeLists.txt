# LibreSprite Scripting Library
# Aseprite     | Copyright (C) 2001-2016  David Capello
# LibreSprite  | Copyright (C)      2021  LibreSprite contributors

include_directories(${DUKTAPE_DIR})
include_directories(${QUICKJS_DIR})

# Debug C/C++ flags
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  if(LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og")
  endif()
else()
  if (LINUX OR APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
  endif()
endif()

add_library(duktape ${DUKTAPE_DIR}/duktape.c)
add_library(script-lib
  duktape/engine.cpp
  v8/engine.cpp
  cout_delegate.cpp)

if(UNIX)
  target_link_libraries(duktape m)
endif()

target_link_libraries(script-lib duktape)
