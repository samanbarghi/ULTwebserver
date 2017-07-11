cmake_minimum_required(VERSION 3.2)
project(newwebserver)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp picohttpparser/picohttpparser.c)
add_executable(newwebserver ${SOURCE_FILES})
target_link_libraries(newwebserver uThreads)
