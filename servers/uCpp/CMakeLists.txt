set(PROJECT ucpp)
set(name uCpp)
if(uCpp)
    SET(UCPPFLAGS "-nodebug -noverify -multi")
    add_definitions(${UCPPFLAGS})
    SET(CMAKE_CXX_COMPILER ${uCpp})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libutserver ${CMAKE_CURRENT_SOURCE_DIR}/../../libutserver/utserver)

    add_custom_target(${name}-http ALL)
    add_custom_command(TARGET ${name}-http PRE_BUILD
        COMMAND ${uCpp} -nodebug -noverify -multi -O3 -std=c++14 -I${CMAKE_CURRENT_SOURCE_DIR}/../../libutserver -I${CMAKE_CURRENT_SOURCE_DIR}/../../libutserver/utserver -o ${name}-http ${CMAKE_CURRENT_SOURCE_DIR}/../../picohttpparser/picohttpparser.c ${CMAKE_CURRENT_SOURCE_DIR}/../../libutserver/src/HTTPSession.cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        COMMENT "Building uC++ server"
        )
endif()

