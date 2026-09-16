# Aseprite Network Library
# Copyright (C) 2001-2015 David Capello

add_library(net-lib
  http_request.cpp
  http_response.cpp)

if(TARGET CURL::libcurl)
  target_link_libraries(net-lib
    PUBLIC CURL::libcurl)
else()
  target_link_libraries(net-lib
    PUBLIC ${CURL_LIBRARIES})
endif()
