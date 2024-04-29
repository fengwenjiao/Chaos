cmake_minimum_required(VERSION 3.13)

project(constellation C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# set Ninja as the default generator
if(NOT CMAKE_GENERATOR)
  set(CMAKE_GENERATOR "Ninja")
endif()

# Set the default build type to Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

option(PSLITE_BUILD_TEST "Build pslite tests" OFF)

if(NOT constellation_LINKER_LIBS)
  set(constellation_LINKER_LIBS "")
endif()

add_library(constellation)

# Add pslite
add_subdirectory("ps-lite-elastic")
list(APPEND constellation_LINKER_LIBS pslite)

file(GLOB_RECURSE SOURCE "src/*.cc" "src/*.h")

target_include_directories(constellation PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(constellation PUBLIC ${constellation_LINKER_LIBS})
target_sources(constellation PRIVATE ${SOURCE})

add_subdirectory(tests)
