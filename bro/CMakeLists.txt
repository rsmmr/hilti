
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if ( BRO_DIST )

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${BRO_DIST}/cmake)
    set(BRO_PLUGIN_BASE "${CMAKE_CURRENT_BINARY_DIR}")

    include(BroPlugin)

    if ( BRO_PLUGIN_ENABLE_DEBUG )
	    set(CMAKE_BUILD_TYPE "Debug")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -DDEBUG")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
	endif()

    if ( "${CMAKE_BUILD_TYPE}" MATCHES "Debug" )
	    # Bro's CMakeConfig checks this and adjusts CMAKE_BUILD_TYPE
		# if this is not set.
        set(ENABLE_DEBUG true)
	endif ()

    set(autogen "${CMAKE_CURRENT_BINARY_DIR}/autogen")
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${autogen})

    if ( BUILDING_HILTI )
        message(STATUS "Building Bro plugin as part of HILTI tree")
        set(binpac "${CMAKE_CURRENT_BINARY_DIR}/../tools/binpac++")
        set(hiltic "${CMAKE_CURRENT_BINARY_DIR}/../tools/hiltic")

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

        set(binpac "XXX")
        set(hiltic "XXX")
    endif ()

    # Autogenerate bro.pac2 C prototypes.
    add_custom_command(
        OUTPUT   ${autogen}/bro.pac2.h
        COMMAND  ${binpac} -P ${CMAKE_CURRENT_SOURCE_DIR}/pac2/bro.pac2 -o ${autogen}/bro.pac2.h
        DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/pac2/bro.pac2 binpac.hlt ${binpac}
        )

    # Autogenerate libbro.hly C prototypes.
    add_custom_command(
        OUTPUT   ${autogen}/libbro.hlt.h
        COMMAND  ${hiltic} -P ${CMAKE_CURRENT_SOURCE_DIR}/pac2/libbro.hlt -o ${autogen}/libbro.hlt.h
        DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/pac2/libbro.hlt ${hiltic}
        )

    # Autogenerate the type information for libbro.hlt.
    add_custom_command(
        OUTPUT   ${autogen}/libbro.hlt.o
        COMMAND  ${hiltic} -b ${CMAKE_CURRENT_SOURCE_DIR}/pac2/libbro.hlt -C -o ${autogen}/libbro.hlt.bc && llc -relocation-model=pic -filetype=obj -o ${autogen}/libbro.hlt.o ${autogen}/libbro.hlt.bc
        DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/pac2/libbro.hlt ${hiltic}
        )

    set_source_files_properties(${autogen}/bro.pac2.h   PROPERTIES GENERATED 1)
    set_source_files_properties(${autogen}/libbro.hlt.h PROPERTIES GENERATED 1)

    bro_plugin_begin(Bro Hilti)
    bro_plugin_cc(src/Converter.cc)
    bro_plugin_cc(src/LocalReporter.cc)
    bro_plugin_cc(src/Manager.cc)
    bro_plugin_cc(src/Pac2AST.cc)
    bro_plugin_cc(src/Pac2Analyzer.cc)
    bro_plugin_cc(src/Pac2FileAnalyzer.cc)
    bro_plugin_cc(src/Plugin.cc)
    bro_plugin_cc(src/Runtime.cc)
    bro_plugin_cc(src/RuntimeInterface.cc)
    bro_plugin_cc(src/compiler/BuilderBase.cc)
    bro_plugin_cc(src/compiler/Compiler.cc)
    bro_plugin_cc(src/compiler/ModuleBuilder.cc)
    bro_plugin_cc(src/compiler/StatementBuilder.cc)
    bro_plugin_cc(src/compiler/ExpressionBuilder.cc)
    bro_plugin_cc(src/compiler/TypeBuilder.cc)
    bro_plugin_cc(src/compiler/ValueBuilder.cc)
    bro_plugin_cc(src/compiler/ConversionBuilder.cc)
    bro_plugin_cc(src/compiler/ASTDumper.cc)
    bro_plugin_cc(src/compiler/Bif.cc)
    bro_plugin_bif(src/consts.bif)
    bro_plugin_bif(src/events.bif)
    bro_plugin_bif(src/functions.bif)
    bro_plugin_obj(${autogen}/bro.pac2.h)
    bro_plugin_obj(${autogen}/libbro.hlt.h)
    bro_plugin_obj(${autogen}/libbro.hlt.o)
    bro_plugin_end()

    if ( BUILDING_HILTI )
        set(plugin_external_libs ${LLVM_LIBS} ${LLVM_LDFLAGS} pcap)
        set(plugin_hilti_libs    hilti hilti-jit ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt-native.a)
        set(plugin_binpac_libs   binpacxx)

        if ( "${CMAKE_BUILD_TYPE}" MATCHES "Debug" )
            set(plugin_hilti_libs  ${plugin_hilti_libs} ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt-dbg.bc)
            set(plugin_binpac_libs ${plugin_binpac_libs} ${PROJECT_BINARY_DIR}/libbinpac/libbinpac-rt-dbg.bc)
        else ()
            set(plugin_hilti_libs  ${plugin_hilti_libs} ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt.bc)
            set(plugin_binpac_libs ${plugin_binpac_libs} ${PROJECT_BINARY_DIR}/libbinpac/libbinpac-rt.bc)
        endif ()

        set(plugin_deps hilti binpacxx libhilti-rt-dbg.bc libbinpac-rt-dbg.bc libhilti-rt.bc libbinpac-rt.bc)

        target_link_libraries(${_plugin_lib} ${plugin_hilti_libs} ${plugin_binpac_libs} ${plugin_external_libs})
        target_link_libraries(${_plugin_lib} ${addl_libs})
        add_dependencies(${_plugin_lib} ${autogen}/bro.pac2.h ${autogen}/libbro.hlt.h ${autogen}/libbro.hlt.o ${plugin_deps})
    else ()
        message(FATAL_ERROR "Building the Bro plugin outside of the HILTI tree is not yet supported.")
    endif ()

    # Turn the build directory into a plugin.
    add_custom_command(TARGET ${_plugin_lib} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/scripts ${CMAKE_CURRENT_BINARY_DIR}/scripts
                       COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/pac2 ${CMAKE_CURRENT_BINARY_DIR}/pac2
                       )

                       # If we copy, changes to {scripts,pac2} won't reflected until the plugin gets relinked.
					   # COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/scripts && mkdir ${CMAKE_CURRENT_BINARY_DIR}/scripts
					   # COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/pac2 && mkdir ${CMAKE_CURRENT_BINARY_DIR}/pac2
                       # COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/scripts ${CMAKE_CURRENT_BINARY_DIR}/scripts
                       # COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/pac2    ${CMAKE_CURRENT_BINARY_DIR}/pac2


else ()
    message(STATUS "Not building Bro plugin")
endif ()
