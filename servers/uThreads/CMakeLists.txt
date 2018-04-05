set(name uThreads)
if(FLASE)
    add_executable(${name}-http ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
    target_link_libraries(${name}-http utserver uThreads pthread ${LD_FLAGS})
endif()

