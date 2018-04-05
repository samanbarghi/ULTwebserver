set(CMAKE_CXX_FLAGS "-std=c++17")

#adding libfibre if gitsubmodule exists
set(FIBREDIR ${CMAKE_CURRENT_SOURCE_DIR}/fibre/KOS/src/libfibre)
if(EXISTS ${FIBREDIR})
    add_subdirectory(fibre)
endif()

#adding qthreads if gitsubmodule exists
set(QTDIR ${CMAKE_CURRENT_SOURCE_DIR}/qthreads/qthreads)
set(QTMAKE ${CMAKE_CURRENT_SOURCE_DIR}/qthreads/qthreads/Makefile.am)
if(EXISTS ${QTMAKE})
    add_subdirectory(qthreads)
endif()





