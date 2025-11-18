# Clip Library
# Copyright (C) 2018-2021 David Capello

function(add_clip_test name)
  add_executable(clip_${name} ${name}.cpp)
  add_test(NAME clip_${name} COMMAND clip_${name})
  target_link_libraries(clip_${name} clip)
  target_include_directories(clip_${name} PUBLIC ..)
  set_tests_properties(clip_${name} PROPERTIES RUN_SERIAL TRUE)
endfunction()

add_clip_test(text_tests)
add_clip_test(user_format_tests)
add_clip_test(image_tests)
