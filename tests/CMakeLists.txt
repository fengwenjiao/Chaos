# 设置目录
set (TEST_OUTPUT "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# pthread flag 
if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# googletest
if(NOT GTEST_ROOT)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/googletest")
        set(GTEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
        set(GTEST_BOTH_LIBRARIES gtest gtest_main)
        set(GTEST_FOUND ON)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gtest")
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/gtest")
        set(GTEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/gtest")
        set(GTEST_BOTH_LIBRARIES gtest gtest_main)
        set(GTEST_FOUND ON)
    else()
        find_package(GTest)
    endif()
else ()
    set(GTEST_FOUND ON)
endif ()

# unit test
if(GTEST_FOUND)
    enable_testing()

    file(GLOB_RECURSE UNIT_TEST_SOURCE "unit_tests/*.cc" "unit_tests/*.h")

    include_directories(${GTEST_INCLUDE_DIR})

    if (NOT PRIVATE_RUNTIME_DIR)
        set(PRIVATE_RUNTIME_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif()

    add_executable(${PROJECT_NAME}_unit_tests ${UNIT_TEST_SOURCE}
            unit_tests/engine_test.cc)

    set_property(TARGET ${PROJECT_NAME}_unit_tests
            PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PRIVATE_RUNTIME_DIR})

    target_link_libraries(${PROJECT_NAME}_unit_tests
            ${GTEST_LIBRARY}
            constellation
            ${pslite_LINKER_LIBS}
    )
    add_test(AllTestsIn${PROJECT_NAME}UnitTests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}_unit_tests)
else ()
    message(FATAL_ERROR "GTest not found")
endif ()


add_executable(test_add_exit test_add_exit.cc)
list(APPEND TEST_OUTPUT test_add_exit)
target_link_libraries(test_add_exit constellation)

# foreach(test_exec ${TEST_OUTPUT})
#     set_target_properties(${test_exec} PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY " ${CMAKE_CURRENT_SOURCE_DIR}")
# endforeach()
