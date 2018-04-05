set(name qthreads)
if(EXISTS ${QTMAKE})
    include_directories(${LIBUTSERVER_INCLUDE_DIRS} . ${QTDIR}/include)
        link_directories(${QTDIR}/src/.libs)

    add_executable(${name}-http ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

    if(NOT EXISTS ${QTDIR}/src/.libs/libqthread.so)
        #run make in qthreads
        add_custom_target(libqthreads
                       COMMAND ./autogen.sh
                       COMMAND ./configure
                       COMMAND make
                       WORKING_DIRECTORY ${QTDIR}
                       COMMENT "Building qthreads"
        )

        add_dependencies(${name}-http libqthreads)
    endif()
    target_link_libraries(${name}-http utserver qthread pthread ${LD_FLAGS})

endif()

