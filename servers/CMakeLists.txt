set(CMAKE_CXX_FLAGS "-std=c++14")

#adding libfibre if gitsubmodule exists
set(FIBREDIR ${CMAKE_CURRENT_SOURCE_DIR}/fibre/KOS/src/libfibre)
if(EXISTS ${FIBREDIR})
    add_subdirectory(fibre)
endif()

# adding uC++
find_program(uCpp u++)
if(uCpp)
    add_subdirectory(uCpp)
endif()


