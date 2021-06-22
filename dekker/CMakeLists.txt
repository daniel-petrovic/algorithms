cmake_minimum_required (VERSION 3.16)

project(dekker)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(dekker dekker.cpp)


