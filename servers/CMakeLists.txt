set(CMAKE_CXX_FLAGS "-std=c++14")


#adding pthreads
add_subdirectory(pthreads)

#adding libfibre if gitsubmodule exists
set(FIBREDIR ${CMAKE_CURRENT_SOURCE_DIR}/fibre/KOS/src/libfibre)
if(EXISTS ${FIBREDIR})
    add_subdirectory(fibre)
endif()

# adding uC++
find_program(uCpp u++)
if(uCpp)
    add_subdirectory(uCpp)
    add_subdirectory(uCpp1)
endif()


