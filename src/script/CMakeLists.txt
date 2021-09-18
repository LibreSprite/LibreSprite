# LibreSprite Scripting Library
# Aseprite     | Copyright (C) 2001-2016  David Capello
# LibreSprite  | Copyright (C)      2021  LibreSprite contributors

include_directories(${DUKTAPE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_library(duktape ${DUKTAPE_DIR}/duktape.c)
add_library(script-lib
  duktape/engine.cpp
  cout_delegate.cpp)

if(UNIX)
  target_link_libraries(duktape m)
endif()

target_link_libraries(script-lib duktape)
