# LibreSprite
# Copyright (C) 2001-2016  David Capello

include_directories(.)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_subdirectory(simpleini)
add_subdirectory(modp_b64)
