# LibreSprite
# Copyright (C) 2001-2016  David Capello

include_directories(.)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_subdirectory(simpleini)
add_subdirectory(modp_b64)
add_subdirectory(quickjs-amalgam)
add_subdirectory(delta)

# Disable observable tests
set(OBSERVABLE_TESTS OFF CACHE BOOL "Compile observable tests")
add_subdirectory(observable)
include_directories(observable)
