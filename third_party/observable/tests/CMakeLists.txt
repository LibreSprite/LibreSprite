# Observable Library
# Copyright (C) 2016-2020 David Capello

function(add_observable_test name)
  add_executable(${name} ${name}.cpp)
  add_test(NAME ${name} COMMAND ${name})
  target_link_libraries(${name} obs)
endfunction()

add_observable_test(adapt_slots)
add_observable_test(count_signals)
add_observable_test(disconnect_on_dtor)
add_observable_test(disconnect_on_rescursive_signal)
add_observable_test(disconnect_on_signal)
add_observable_test(multithread)
add_observable_test(multithread_futures)
add_observable_test(observers)
add_observable_test(reconnect_on_notification)
add_observable_test(reconnect_on_signal)
add_observable_test(signals)
