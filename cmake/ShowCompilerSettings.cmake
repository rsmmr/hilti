
MESSAGE(STATUS "Final values for build mode ${CMAKE_BUILD_TYPE}:")

if ( "${CCC_CC}" STREQUAL "" )
    set(CCC_CC "not set")
endif ()

if ( "${CCC_CXX}" STREQUAL "" )
    set(CCC_CXX "not set")
endif ()

MESSAGE(STATUS "    C_COMPILER:         ${CMAKE_C_COMPILER}   [CCC_CC:  ${CCC_CC}]")
MESSAGE(STATUS "    CXX_COMPILER:       ${CMAKE_CXX_COMPILER} [CCC_CXX: ${CCC_CXX}]")
MESSAGE(STATUS "    C_FLAGS:            ${CMAKE_C_FLAGS}")
MESSAGE(STATUS "    CXX_FLAGS:          ${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "    CMAKE_ASM_COMPILER: ${CMAKE_ASM_COMPILER}")
MESSAGE(STATUS "    CMAKE_AR:           ${CMAKE_AR}")
MESSAGE(STATUS "    CMAKE_RANLIB:       ${CMAKE_RANLIB}")
MESSAGE(STATUS "    CMAKE_LD:           ${CMAKE_LD}")
