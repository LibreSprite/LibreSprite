# SHE
# Copyright (C) 2012-2016  David Capello

set(SHE_SOURCES
  common/freetype_font.cpp)

######################################################################
# Allegro 4 backend

if(USE_ALLEG4_BACKEND)
  add_definitions(-DUSE_KEY_POLLER)
  if(WIN32)
    find_package(DXGuid)
    if(NOT DXGUID_FOUND)
      if(MSVC)
        message(FATAL_ERROR "DirectX required for Windows port. You might need to add DirectX include and lib directories to your INCLUDE and LIB environment variables.")
      else()
        message(FATAL_ERROR "DirectX required for Windows port.")
      endif()
    endif()
    include_directories(SYSTEM ${DXGUID_INCLUDE_DIR})
  else()
    add_definitions(-DUSE_MOUSE_POLLER)
  endif()

  list(APPEND SHE_SOURCES
    alleg4/alleg_display.cpp
    alleg4/alleg_surface.cpp
    alleg4/display_events.cpp
    alleg4/key_poller.cpp
    alleg4/mouse_poller.cpp
    alleg4/she.cpp)

  if(APPLE)
    if(NOT USE_SHARED_ALLEGRO4)
      list(APPEND SHE_SOURCES alleg4/app.mm)
    endif()
  endif()
endif()

######################################################################
# SDL2 backend

if(USE_SDL2_BACKEND)
  list(APPEND SHE_SOURCES
    sdl2/sdl2_display.cpp
    sdl2/sdl2_surface.cpp
    sdl2/she.cpp)
endif()

######################################################################

if(APPLE)
  list(APPEND SHE_SOURCES
    osx/logger.mm)
endif()

add_library(she ${SHE_SOURCES})

target_link_libraries(she
  gfx-lib
  base-lib
  ${FREETYPE_LIBRARIES})

if(USE_ALLEG4_BACKEND)
  target_link_libraries(she
    ${LOADPNG_LIBRARY}
    ${LIBALLEGRO4_LINK_FLAGS}
    ${DXGUID_LIBRARIES}
    ${X11_LIBRARIES})
endif()

if(USE_SDL2_BACKEND)
  target_link_libraries(she ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
  target_compile_options(she PUBLIC ${SDL2_CFLAGS_OTHER})
endif()
