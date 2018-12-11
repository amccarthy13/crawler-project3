cmake_minimum_required(VERSION 3.12)
project(untitled)

set(CMAKE_CXX_STANDARD 14)

add_executable(untitled mcrawler2.cpp socket.h parser.h parser.cpp socket.cpp mcrawler1.cpp)