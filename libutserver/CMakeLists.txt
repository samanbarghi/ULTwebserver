set(CMAKE_CXX_FLAGS "-std=c++17")
project(libutserver VERSION 0.1.0 LANGUAGES CXX)

set(CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(GCC_COVERAGE_COMPILE_FLAGS "-m64 -fpermissive -mtls-direct-seg-refs -fno-extern-tls-init")

file(GLOB_RECURSE LIBUTSERVER_SRCS src  "*.cpp")
file(GLOB_RECURSE LIBUTSERVER_HDRS utserver "*.h")

SET_SOURCE_FILES_PROPERTIES(../picohttpparser/picohttpparser.c PROPERTIES LANGUAGE CXX )
add_library(utserver STATIC ../picohttpparser/picohttpparser.c ${LIBUTSERVER_SRCS})

target_include_directories(utserver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utserver>
    $<INSTALL_INTERFACE:include/utserver>
    PRIVATE src)

# If we have compiler requirements for this library, list them
# here
#target_compile_features(utserver
#    PRIVATE cxx_range_for)



# 'make install' to the correct location
install(TARGETS utserver EXPORT utserverConfig
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin)  # This is for Windows
install(DIRECTORY utserver/ DESTINATION include/utserver)

# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into <prefix>/cmake.
install(EXPORT utserverConfig DESTINATION share/utserver/cmake)

# This makes the project importable from the build directory
export(TARGETS utserver FILE utserverConfig.cmake)


#set(SOURCE_FILES main.cpp picohttpparser/picohttpparser.c libutserver/utserver/HTTPSession.cpp)
#add_executable(newwebserver ${SOURCE_FILES})
#target_link_libraries(newwebserver uThreads)

