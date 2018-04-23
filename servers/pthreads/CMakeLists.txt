set(name pthreads)
set(project pthreads)

include_directories(${LIBUTSERVER_INCLUDE_DIRS} .)
add_executable(${name}-http ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(${name}-http utserver pthread ${LD_FLAGS})

