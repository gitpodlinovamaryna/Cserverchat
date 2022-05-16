cmake_minimum_required(VERSION 3.0.0)

project(servermult)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(SRC_LIST tcpserver.c)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)

add_executable(${PROJECT_NAME} ${SRC_LIST})