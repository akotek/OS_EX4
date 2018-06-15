cmake_minimum_required(VERSION 3.8)
project(OS_EX4)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES whatsappServer.cpp whatsappio.cpp whatsappio.h Utils.cpp Utils.h)
add_executable(OS_EX4 ${SOURCE_FILES})