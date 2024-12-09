cmake_minimum_required(VERSION 3.13)

project(resource_monitor C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS ON)

# set Ninja as the default generator
if(NOT CMAKE_GENERATOR)
  set(CMAKE_GENERATOR "Ninja")
endif()

if(NOT resource_monitor_LINKER_LIBS)
  set(resource_monitor_LINKER_LIBS "")
endif()
if(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

add_library(resource_monitor)


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/iperf-cmake)

# add_executable(resource_monitor main.cc)
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/iperf-cmake/iperf/src", "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/json/")
target_include_directories(resource_monitor PRIVATE "LIBIPERF_HDRS")
target_include_directories(resource_monitor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/json/")
# include_directories($LIBIPERF_HDRS, "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/json/")
target_link_libraries(resource_monitor PRIVATE iperf)
target_precompile_headers(resource_monitor PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/iperf-cmake/iperf_config.h)

# link_directories( /home/zzh/resource_monitor/3rdparty/iperf/src/.libs)

# target_link_libraries(resource_monitor libiperf.so)

# build
file(GLOB_RECURSE SOURCE "src/*.cc")

target_include_directories(resource_monitor PUBLIC "${PROJECT_SOURCE_DIR}/include/clusterRM")
target_link_libraries(resource_monitor PUBLIC ${resource_monitor_LINKER_LIBS})
target_sources(resource_monitor PRIVATE ${SOURCE})

# add_definitions(-DDEBUG)

add_subdirectory(tests)
if (CMQ_BUILD_TESTS)
  add_subdirectory(tests)
endif()



