set(name libfibre)
if(EXISTS ${FIBREDIR})
    include_directories(${LIBUTSERVER_INCLUDE_DIRS} . ${FIBREDIR}/..)
    link_directories(${FIBREDIR})

	#run make in libfibre
	add_custom_target(libfibre
                   COMMAND make
                   WORKING_DIRECTORY ${FIBREDIR}
                   COMMENT "Building libfibre"
	)

    add_executable(${name}-http ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
    add_dependencies(${name}-http libfibre)
    target_link_libraries(${name}-http utserver fibre pthread ${LD_FLAGS})

endif()

