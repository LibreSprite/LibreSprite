# Locate V8
# This module defines
# V8_LIBRARY
# V8_FOUND, if false, do not try to link to gdal
# V8_INCLUDE_DIR, where to find the headers
#
# $V8_DIR is an environment variable that would
# correspond to the ./configure --prefix=$V8_DIR
#
# Created by Robert Osfield (based on FindFLTK.cmake)

FIND_PATH(V8_INCLUDE_DIR v8.h
    $ENV{V8_DIR}/include
    $ENV{V8_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /usr/include/nodejs/deps/v8/include
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    /usr/freeware/include
)

FIND_LIBRARY(V8_LIBRARY
    NAMES v8 libv8
    PATHS
    $ENV{V8_DIR}/lib
    $ENV{V8_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/lib
    /usr/lib
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    /usr/freeware/lib64
)

set(V8_LIBRARY_PATHS
    $ENV{V8_DIR}/lib
    $ENV{V8_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/lib
    /usr/lib
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    /usr/freeware/lib64
)

FIND_LIBRARY(V8_LIBRARY NAMES v8 libv8 PATHS ${V8_LIBRARY_PATHS})
FIND_LIBRARY(V8_LIBBASE_LIBRARY NAMES v8_libbase)
FIND_LIBRARY(V8_LIBPLATFORM_LIBRARY NAMES v8_libplatform)

SET(V8_FOUND "NO")
IF(V8_LIBBASE_LIBRARY AND V8_LIBPLATFORM_LIBRARY AND V8_LIBRARY AND V8_INCLUDE_DIR)
    SET(V8_FOUND "YES")
    set(V8_LIBRARIES ${V8_LIBBASE_LIBRARY} ${V8_LIBPLATFORM_LIBRARY} ${V8_LIBRARY})
ENDIF()
