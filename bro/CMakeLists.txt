
if ( BRO_DIST )

    if ( NOT BRO_DIST )
        message(FATAL_ERROR "BRO_DIST not set")
    endif ()

    set(CMAKE_MODULE_PATH ${BRO_DIST}/cmake)

    if ( "${CMAKE_BUILD_TYPE}" MATCHES "Debug" )
	    # Bro's CMakeConfig checks this and adjusts CMAKE_BUILD_TYPE
		# if this is not set.
        set(ENABLE_DEBUG true)
	endif ()

    include(BroPlugin)

    if ( BUILDING_HILTI )
        message(STATUS "Building Bro plugin as part of HILTI tree")

    else ()
        project(Bro-Plugin-Hilti)

        cmake_minimum_required(VERSION 2.6.3)

        find_package(HILTI)

        if ( NOT HILTI_FOUND )
            message(FATAL_ERROR "HILTI not found")
        endif()

        message(STATUS "Building Bro plugin outsidoe of of HILTI tree")

        set(CMAKE_C_COMPILER          "${HILTI_CLANG_EXEC}")
        set(CMAKE_CXX_COMPILER        "${HILTI_CLANGXX_EXEC}")
        set(CMAKE_C_FLAGS             "${CMAKE_C_FLAGS} ${HILTI_C_FLAGS}")
        set(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} ${HILTI_CXX_FLAGS}")
        set(CMAKE_LD_FLAGS            "${CMAKE_LD_FLAGS} ${HILTI_LD_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_LD_FLAGS} ${HILTI_LD_FLAGS}")
    endif ()

    # Make sure the library ends up in the right directory where Bro
    # will find it.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/dylib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/dylib")

    bro_plugin_begin(Bro Hilti)
    bro_plugin_cc(src/Converter.cc)
    bro_plugin_cc(src/LocalReporter.cc)
    bro_plugin_cc(src/Manager.cc)
    bro_plugin_cc(src/Pac2AST.cc)
    bro_plugin_cc(src/Pac2Analyzer.cc)
    bro_plugin_cc(src/Plugin.cc)
    bro_plugin_cc(src/Runtime.cc)
    bro_plugin_bif(src/consts.bif)
    bro_plugin_bif(src/events.bif)
    bro_plugin_bif(src/functions.bif)
    bro_plugin_end()

    if ( BUILDING_HILTI )
        set(plugin_external_libs ${LLVM_LIBS} ${LLVM_LDFLAGS} pcap ${PAPI})
        set(plugin_hilti_libs    hilti hilti-jit ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt-native.a)
        set(plugin_binpac_libs   binpacxx)

        if ( "${CMAKE_BUILD_TYPE}" MATCHES "Debug" )
            set(plugin_hilti_libs  ${plugin_hilti_libs} ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt-dbg.bc)
            set(plugin_binpac_libs ${plugin_binpac_libs} ${PROJECT_BINARY_DIR}/libbinpac/libbinpac-rt-dbg.bc)
            set(plugin_deps libhilti-rt-dbg.bc libbinpac-rt-dbg.bc)
        else ()
            set(plugin_hilti_libs  ${plugin_hilti_libs} ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt.bc)
            set(plugin_binpac_libs ${plugin_binpac_libs} ${PROJECT_BINARY_DIR}/libbinpac/libbinpac-rt.bc)
            set(plugin_deps libhilti-rt.bc libbinpac-rt.bc)
        endif ()

        target_link_libraries(${_plugin_lib} ${plugin_hilti_libs} ${plugin_binpac_libs} ${plugin_external_libs})
        add_dependencies(${_plugin_lib} ${plugin_deps})
    else ()
        message(FATAL_ERROR "Building the Bro plugin outside of the HILTI tree is not yet supported.")
    endif ()

    # Turn the build directory into a plugin.
    add_custom_command(TARGET ${_plugin_lib} POST_BUILD
                       COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/__bro_plugin__
					   COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/scripts && mkdir ${CMAKE_CURRENT_BINARY_DIR}/scripts
					   COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/pac2 && mkdir ${CMAKE_CURRENT_BINARY_DIR}/pac2
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/scripts ${CMAKE_CURRENT_BINARY_DIR}/scripts
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/pac2    ${CMAKE_CURRENT_BINARY_DIR}/pac2)

else ()
    message(STATUS "Not building Bro plugin")
endif ()
