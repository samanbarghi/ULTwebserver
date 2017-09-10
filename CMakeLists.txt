cmake_minimum_required(VERSION 3.2)
SET ( CMAKE_CXX_FLAGS "-std=c++17" )
project(newwebserver)

#set(CMAKE_CXX_STANDARD 1z)

set(SOURCE_FILES main.cpp picohttpparser/picohttpparser.c libutserver/utserver/HTTPSession.cpp)
add_executable(newwebserver ${SOURCE_FILES})
target_link_libraries(newwebserver uThreads)

