cmake_minimum_required(VERSION 3.13)

project(constellation C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

option(CONSTEL_NETWORK_AWARE "Build network aware" ON)

option(CONSTEL_BUILD_SHARED_LIBS "Build shared libraries instead of static libraries" ON)

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

# set Ninja as the default generator
if (NOT CMAKE_GENERATOR)
    set(CMAKE_GENERATOR "Ninja")
endif ()

# Set the default build type to Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

if (NOT constellation_LINKER_LIBS)
    set(constellation_LINKER_LIBS "")
endif ()

# Add pslite
option(PSLITE_BUILD_TEST "Build pslite tests" OFF)

add_subdirectory("ps-lite-elastic")
#set_target_properties(pslite PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
list(APPEND constellation_LINKER_LIBS pslite)

#Add network aware
option(CMQ_BUILD_TESTS "Build network aware tests" OFF)
if (CONSTEL_NETWORK_AWARE)
    add_subdirectory("ClusterResourceMonitor")
    add_definitions(-DCONS_NETWORK_AWARE)
    list(APPEND constellation_LINKER_LIBS resource_monitor)
endif ()

# Add gtest
set(GTEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/googletest/")
set(GTEST_INCLUDE_DIR ${GTEST_ROOT}/include)
set(GTEST_LIBRARIES gtest gtest_main)
set(GTEST_MAIN_LIBRARY gtest_main)
set(GTEST_LIBRARY gtest)

add_subdirectory(${GTEST_ROOT})

if (CONSTEL_BUILD_SHARED_LIBS)
    add_library(constellation SHARED)
else ()
    add_library(constellation STATIC)
endif ()

file(GLOB_RECURSE SOURCE "src/*.cc")

target_include_directories(constellation PUBLIC "${PROJECT_SOURCE_DIR}/include/")
target_include_directories(constellation PUBLIC "${PROJECT_SOURCE_DIR}/ps-lite-elastic/include/")
target_include_directories(constellation PUBLIC "${PROJECT_SOURCE_DIR}/ClusterResourceMonitor/include/")
target_link_libraries(constellation PUBLIC ${constellation_LINKER_LIBS})
target_sources(constellation PRIVATE ${SOURCE})

add_subdirectory(tests)
