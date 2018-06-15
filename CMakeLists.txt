cmake_minimum_required(VERSION 3.8)
project(OS_EX4)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp whatsappio.h whatsappio.cpp example_with_select_server.cpp)
add_executable(OS_EX4 ${SOURCE_FILES})