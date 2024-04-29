# 设置目录
set (TEST_OUTPUT "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# pthread flag 
if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

add_executable(test_add_exit test_add_exit.cc)
list(APPEND TEST_OUTPUT test_add_exit)
target_link_libraries(test_add_exit constellation)

# foreach(test_exec ${TEST_OUTPUT})
#     set_target_properties(${test_exec} PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY " ${CMAKE_CURRENT_SOURCE_DIR}")
# endforeach()
