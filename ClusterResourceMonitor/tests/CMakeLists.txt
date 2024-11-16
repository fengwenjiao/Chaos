set (TEST_OUTPUT "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# add_executable(test_iperf test_iperf.cc)
# target_link_libraries(test_iperf resource_monitor)

# add_executable(test_iperf_api test_iperf_api.cc)
# target_link_libraries(test_iperf_api resource_monitor)

# add_executable(test_static_info test_static_info.cc)
# target_link_libraries(test_static_info resource_monitor)

# add_executable(test_dynamic_info test_dynamic_info.cc)
# target_link_libraries(test_dynamic_info resource_monitor)

# add_executable(test_smq_server test_smq_server.cc)
# target_link_libraries(test_smq_server resource_monitor)

# add_executable(test_smq_client test_smq_client.cc)
# target_link_libraries(test_smq_client resource_monitor)

# add_executable(test_json test_json.cc)
# target_link_libraries(test_json resource_monitor)

set(tests test_iperf test_iperf_api test_static_info test_dynamic_info test_smq_server test_smq_client)

foreach(test ${tests})
  add_executable(${test}.out ${test}.cc)
  target_link_libraries(${test}.out resource_monitor)
  target_compile_definitions(${test}.out PRIVATE DEBUG)
endforeach()




target_compile_definitions(resource_monitor PRIVATE DEBUG)