# cmake support for cpp-httplib (vendored, MIT license)

add_library(httplib
  httplib.cpp
)

target_include_directories(httplib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(httplib PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
target_link_libraries(httplib PUBLIC Threads::Threads)

# https support; the build falls back to plain http when OpenSSL is absent
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
  target_compile_definitions(httplib PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
  target_link_libraries(httplib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()
