# cmake support for delta

add_library(delta
  JSON.cpp
  QuickJSInterpreter.cpp
)

target_include_directories(delta PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(delta PUBLIC ${QUICKJS_DIR})
set_target_properties(delta PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(delta PUBLIC quickjs)
