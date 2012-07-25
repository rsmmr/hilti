#
# If we compile in debugging mode, we activate clang's debugging capabilities:
#
#   - The static analyzer if we find 'ccc-analyzer'
#   - AddressSanitizer [Disabled for now, produces linker errors; probably need compiler-rt]
#

if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    find_program(ccc_analyzer "ccc-analyzer")
    find_program(ccc_analyzer_wrapper_in "ccc-analyzer-cmake-wrapper.in" HINTS ${CMAKE_MODULE_PATH})

    if ( NOT "${ccc_analyzer}" STREQUAL "ccc_analyzer-NOTFOUND" )
        message(STATUS "Activating clang's static analyzer for C; original compiler is ${CMAKE_CXX_COMPILER}.")

        set(CCC_CC "${CMAKE_C_COMPILER}")

        set(ccc_analyzer_wrapper "${CMAKE_CURRENT_BINARY_DIR}/ccc-analyzer-cmake-wrapper")
        configure_file(${ccc_analyzer_wrapper_in} ${ccc_analyzer_wrapper})

        set(CMAKE_C_COMPILER "${ccc_analyzer_wrapper}")

    else ()
        message(STATUS "NOTE: Cannot find ccc-analyzer in PATH, no static analysis for C")
    endif()

    find_program(cxx_analyzer "c++-analyzer")
    find_program(cxx_analyzer_wrapper_in "c++-analyzer-cmake-wrapper.in" HINTS ${CMAKE_MODULE_PATH})

    if ( NOT "${cxx_analyzer}" STREQUAL "cxx_analyzer-NOTFOUND" )
        message(STATUS "Activating clang's static analyzer for C++; original compiler is ${CMAKE_CXX_COMPILER}.")

        set(CCC_CXX "${CMAKE_C_COMPILER}")

        set(cxx_analyzer_wrapper "${CMAKE_CURRENT_BINARY_DIR}/c++-analyzer-cmake-wrapper")
        configure_file(${cxx_analyzer_wrapper_in} ${cxx_analyzer_wrapper})

        set(CMAKE_C_COMPILER "${cxx_analyzer_wrapper}")

    else ()
        message(STATUS "NOTE: Cannot find c++-analyzer in PATH, no static analysis for C++")
    endif()


    # set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -faddress-sanitizer")

endif ()
