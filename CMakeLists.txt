cmake_minimum_required(VERSION 3.14)

project(ioringcpp)

add_library(ioringcpp INTERFACE)
target_include_directories(ioringcpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(echo_server examples/echo_server.cpp)
target_link_libraries(echo_server PRIVATE ioringcpp)

add_executable(echo_client examples/echo_client.cpp)
target_link_libraries(echo_client PRIVATE ioringcpp)
