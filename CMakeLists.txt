cmake_minimum_required(VERSION 2.8)

project(metamethod)
set (CMAKE_CXX_FLAGS "-std=c++17")

set(SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metatype.cpp
    )
set(HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/metaclass.h
    ${CMAKE_CURRENT_SOURCE_DIR}/metatype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/function_traits.h
    ${CMAKE_CURRENT_SOURCE_DIR}/arguments.h
    ${CMAKE_CURRENT_SOURCE_DIR}/invokers.h
    )
add_executable(${PROJECT_NAME} ${SOURCE} ${HEADER})
