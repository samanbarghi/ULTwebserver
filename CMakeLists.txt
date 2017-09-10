cmake_minimum_required(VERSION 3.2)
project(utserver VERSION 0.1.0 LANGUAGES CXX)

set(CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIBUTSERVER_INCLUDE_DIRS
     "${CMAKE_CURRENT_SOURCE_DIR}/libutserver")

add_subdirectory(libutserver)
add_subdirectory(servers)
