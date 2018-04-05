set(CMAKE_CXX_FLAGS "-std=c++17")

#include libfibre
set(FIBREDIR ${CMAKE_CURRENT_SOURCE_DIR}/fibre/KOS/src/libfibre)
if(EXISTS ${FIBREDIR})
    add_subdirectory(fibre)
endif()
