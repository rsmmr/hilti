#
# If we compile in debugging mode, we activate clang's debugging capabilities:
#
#   - The static analyzer if we find 'ccc-analyzer'
#
#   - On demand, one of the sanitizers:
#     -DCLANG_SANITIZE=address  Address sanitizer
#     -DCLANG_SANITIZE=thread   Thread sanitizer
#     -DCLANG_SANITIZE=memory   Memory sanitizer (w/ origin tracking enabled)
#
#     TODO: Activating the sanitizer doesn't work. The problem is that
#     it needs the compilter-rt runtime library, and that needs to be
#     linked first and with "-whole-archive <lib> -no-whole-archive".
#     Normally clang takes care of that if it gets the -fsanitizer=...
#     flag, but we're doing the linking ourselves (for JIT at least;
#     for the static binaries we could pass the option through to clang).

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

        set(CCC_CXX "${CMAKE_CXX_COMPILER}")

        set(cxx_analyzer_wrapper "${CMAKE_CURRENT_BINARY_DIR}/c++-analyzer-cmake-wrapper")
        configure_file(${cxx_analyzer_wrapper_in} ${cxx_analyzer_wrapper})

        set(CMAKE_CXX_COMPILER "${cxx_analyzer_wrapper}")

    else ()
        message(STATUS "NOTE: Cannot find c++-analyzer in PATH, no static analysis for C++")
    endif()

    ### Enable selected sanitizer.
    ###
    ### NOTE: Doesn't work, see above.

    if ( "${CLANG_SANITIZE}" STREQUAL "memory" )
        set(sanaddl "-fsanitize-memory-track-origins")
    endif ()

   if ( CLANG_SANITIZE )
       set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS}  -fsanitize=${CLANG_SANITIZE} ${sanaddl}")
       set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -fsanitize=${CLANG_SANITIZE} ${sanaddl}")
   endif ()
endif ()
