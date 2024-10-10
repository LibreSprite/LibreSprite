# SHE
# Copyright (C) 2012-2016  David Capello

set(SHE_SOURCES
  common/freetype_font.cpp)

include_directories(${EASYTAB_DIR})

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

######################################################################
# SDL2 backend

if(USE_SDL2_BACKEND)
  list(APPEND SHE_SOURCES
    sdl2/sdl2_display.cpp
    sdl2/sdl2_surface.cpp
    sdl2/she.cpp)
endif()

######################################################################

if(WIN32)
  list(APPEND SHE_SOURCES
    win/native_dialogs.cpp)
endif()

if(APPLE)
  list(APPEND SHE_SOURCES
    osx/logger.mm
    osx/native_dialogs.mm
    osx/tablet.mm)
endif()

if(WITH_GTK_FILE_DIALOG_SUPPORT AND UNIX AND NOT APPLE AND NOT BEOS)
  if(WITH_DEPRECATED_GLIB_SUPPORT)
    add_definitions(-DASEPRITE_DEPRECATED_GLIB_SUPPORT)
  endif()
  add_definitions(-DASEPRITE_WITH_GTK_FILE_DIALOG_SUPPORT)
  list(APPEND SHE_SOURCES
    gtk/native_dialogs.cpp)
endif()

add_library(she ${SHE_SOURCES})

target_link_libraries(she
  gfx-lib
  base-lib
  ${FREETYPE_LIBRARIES})

if(USE_SDL2_BACKEND)
  target_link_libraries(she ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
  target_compile_options(she PUBLIC ${SDL2_CFLAGS_OTHER})
endif()

if(WITH_GTK_FILE_DIALOG_SUPPORT)
  target_link_libraries(she
    ${GTKMM_LIBRARIES})
endif()
