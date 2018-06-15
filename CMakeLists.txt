cmake_minimum_required(VERSION 3.8)
project(OS_EX4)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES whatsappClient.cpp whatsappServer.cpp whatsappio.cpp whatsappio.h Utils.cpp)
add_executable(OS_EX4 ${SOURCE_FILES})