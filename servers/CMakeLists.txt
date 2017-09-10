set(CMAKE_CXX_FLAGS "-std=c++17")
macro(add_server name deplib)
  add_executable(${name}-http ${CMAKE_CURRENT_SOURCE_DIR}/${name}/main.cpp)

  target_link_libraries(${name}-http ${deplib} utserver ${LD_FLAGS})
endmacro()
include_directories(${LIBUTSERVER_INCLUDE_DIRS} .)

#SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/http-parser/http_parser.c PROPERTIES LANGUAGE CXX )

add_server(uThreads uThreads)