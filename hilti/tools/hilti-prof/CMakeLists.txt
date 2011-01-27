# $Id$

cmake_minimum_required(VERSION 2.8)

project(hilti-prof)
set(PROJECT_VERSION "0.1")

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../hilti-build2/hilti-build2
    ARGS    -m -o ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c
    )

add_custom_target(hilti-prof ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof)






































