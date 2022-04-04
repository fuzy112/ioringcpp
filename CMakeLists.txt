cmake_minimum_required(VERSION 3.14)

project(ioringcpp)

add_library(ioringcpp INTERFACE)
target_include_directories(ioringcpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test1 examples/main.cpp)
target_link_libraries(test1 ioringcpp)