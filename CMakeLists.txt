cmake_minimum_required(VERSION 2.8)
project(GDNS)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/misc)

find_package(LibUV REQUIRED)
include_directories(${LIBUV_INCLUDE_DIRS})

find_package(LibConfig REQUIRED)
include_directories(${LIBCONFIGPP_INCLUDE_DIR})

include_directories(vendor)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

enable_testing()

add_subdirectory(src)
add_subdirectory(test)