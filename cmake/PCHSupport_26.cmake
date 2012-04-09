# - Try to find precompiled headers support for GCC 3.4 and 4.x
# Once done this will define:
#
# Variable:
#   PCHSupport_FOUND
#
# Macro:
#   ADD_PRECOMPILED_HEADER  _targetName _input  _dowarn

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

SET(PCHSupport_FOUND TRUE)
SET(_PCH_include_prefix -I)

MACRO(_PCH_GET_COMPILE_FLAGS _out_compile_flags)
    SET(${_out_compile_flags} ${CMAKE_CXX_FLAGS} )

    STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
    if(${_flags_var_name})
        list(APPEND ${_out_compile_flags} ${${_flags_var_name}} )
    endif(${_flags_var_name})

    # If the compiler is g++ and the target type is shared library, we have
    # to add -fPIC to its compile flags.
        GET_TARGET_PROPERTY(_targetType ${_PCH_current_target} TYPE)
        IF(${_targetType} STREQUAL SHARED_LIBRARY)
            LIST(APPEND ${_out_compile_flags} -fPIC)
        ENDIF(${_targetType} STREQUAL SHARED_LIBRARY)

    # Add all include directives...
    GET_DIRECTORY_PROPERTY(DIRINC INCLUDE_DIRECTORIES )
    FOREACH(item ${DIRINC})
        LIST(APPEND ${_out_compile_flags} ${_PCH_include_prefix}${item})
    ENDFOREACH(item)

    # Add all definitions...
    GET_TARGET_PROPERTY(_compiler_flags ${_PCH_current_target} COMPILE_FLAGS)
	if(_compiler_flags)
		LIST(APPEND ${_out_compile_flags} ${_compiler_flags})
	endif(_compiler_flags)

    GET_DIRECTORY_PROPERTY(_directory_flags COMPILE_DEFINITIONS)
	if(_directory_flags)
		foreach(flag ${_directory_flags})
			LIST(APPEND ${_out_compile_flags} -D${flag})
		endforeach(flag)
	endif(_directory_flags)

    STRING(TOUPPER "COMPILE_DEFINITIONS_${CMAKE_BUILD_TYPE}" _defs_prop_name)
    GET_DIRECTORY_PROPERTY(_directory_flags ${_defs_prop_name})
	if(_directory_flags)
		foreach(flag ${_directory_flags})
			LIST(APPEND ${_out_compile_flags} -D${flag})
		endforeach(flag)
	endif(_directory_flags)

    get_target_property(_target_flags ${_PCH_current_target} COMPILE_DEFINITIONS)
	if(_target_flags)
		foreach(flag ${_target_flags})
			LIST(APPEND ${_out_compile_flags} -D${flag})
		endforeach(flag)
	endif(_target_flags)

    get_target_property(_target_flags ${_PCH_current_target} ${_defs_prop_name})
	if(_target_flags)
		foreach(flag ${_target_flags})
			LIST(APPEND ${_out_compile_flags} -D${flag})
		endforeach(flag)
	endif(_target_flags)

    SEPARATE_ARGUMENTS(${_out_compile_flags})
ENDMACRO(_PCH_GET_COMPILE_FLAGS)


MACRO(_PCH_GET_COMPILE_COMMAND out_command _input _output)
	FILE(TO_NATIVE_PATH ${_input} _native_input)
	FILE(TO_NATIVE_PATH ${_output} _native_output)

        IF(CMAKE_CXX_COMPILER_ARG1)
	        # remove leading space in compiler argument
            STRING(REGEX REPLACE "^ +" "" pchsupport_compiler_cxx_arg1 ${CMAKE_CXX_COMPILER_ARG1})
        ELSE(CMAKE_CXX_COMPILER_ARG1)
            SET(pchsupport_compiler_cxx_arg1 "")
        ENDIF(CMAKE_CXX_COMPILER_ARG1)
        SET(${out_command} 
#            ${CMAKE_CXX_COMPILER} ${pchsupport_compiler_cxx_arg1} ${_compile_FLAGS}	-x c++-header -o ${_output} ${_input})
            ${CMAKE_CXX_COMPILER} ${pchsupport_compiler_cxx_arg1} ${_compile_FLAGS}	-x c++-header  ${_input})
ENDMACRO(_PCH_GET_COMPILE_COMMAND )

MACRO(_PCH_GET_TARGET_COMPILE_FLAGS _cflags  _header_name _pch_path _dowarn )
    FILE(TO_NATIVE_PATH ${_pch_path} _native_pch_path)
#message(${_native_pch_path})

        # for use with distcc and gcc>4.0.1 if preprocessed files are accessible
        # on all remote machines set
        # PCH_ADDITIONAL_COMPILER_FLAGS to -fpch-preprocess
        # if you want warnings for invalid header files (which is very 
        # inconvenient if you have different versions of the headers for
        # different build types you may set _pch_dowarn
        LIST(APPEND ${_cflags} ${PCH_ADDITIONAL_COMPILER_FLAGS})
		IF (_dowarn)
            LIST(APPEND ${_cflags} -Winvalid-pch)
		ENDIF ()

	get_target_property(_old_target_cflags ${_PCH_current_target} COMPILE_FLAGS)
	if(_old_target_cflags)
		list(APPEND ${_cflags} ${_old_target_cflags})
	endif(_old_target_cflags)

    STRING(REPLACE ";" " " ${_cflags} "${${_cflags}}")
ENDMACRO(_PCH_GET_TARGET_COMPILE_FLAGS )

MACRO(GET_PRECOMPILED_HEADER_OUTPUT _targetName _input _output _outdirprefix)
    GET_FILENAME_COMPONENT(_name ${_input} NAME)
        set(_build "")
        IF(CMAKE_BUILD_TYPE)
            set(_build _${CMAKE_BUILD_TYPE})
        endif(CMAKE_BUILD_TYPE)
		file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${_outdirprefix}/${_name}.gch)
        SET(_output "${CMAKE_CURRENT_BINARY_DIR}/${_outdirprefix}/${_name}.gch/${_targetName}${_build}.h++")
ENDMACRO(GET_PRECOMPILED_HEADER_OUTPUT)

MACRO(ADD_PRECOMPILED_HEADER _targetName _input _dependson)
    SET(_PCH_current_target ${_targetName})

	if(${ARGN})
		set(_dowarn 1)
	else(${ARGN})
		set(_dowarn 0)
	endif(${ARGN})

    GET_FILENAME_COMPONENT(_name ${_input} NAME)
    GET_FILENAME_COMPONENT(_path ${_input} PATH)
    # GET_PRECOMPILED_HEADER_OUTPUT( ${_targetName} ${_input} _output ${_outdirprefix})

    #GET_FILENAME_COMPONENT(_outdir ${_output} PATH )

    #MESSAGE(OUT: ${_outdir} )
    #MESSAGE(OUT: ${_output} )
    #FILE(MAKE_DIRECTORY ${_outdir})

    _PCH_GET_COMPILE_FLAGS(_compile_FLAGS)

    # Ensure same directory! Required by gcc
    IF(NOT ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
        SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/${_name} PROPERTIES GENERATED 1)
        ADD_CUSTOM_COMMAND(OUTPUT	${CMAKE_CURRENT_BINARY_DIR}/${_name} 
#                           COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_CURRENT_SOURCE_DIR}/${_input} ${CMAKE_CURRENT_BINARY_DIR}/${_name}
                           COMMAND ${CMAKE_COMMAND} -E copy  ${_input} ${CMAKE_CURRENT_BINARY_DIR}/${_name}
                           DEPENDS ${_input})
    ENDIF(NOT ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})

#message("_command  ${_input} ${_output}")
    _PCH_GET_COMPILE_COMMAND(_command  ${_input} ${_targetName} )

    GET_FILENAME_COMPONENT(basename ${_input} NAME)
    GET_FILENAME_COMPONENT(outdir ${_targetName} PATH)
    set(copy "${outdir}/${basename}")

    #message("target:  ${_targetName}")
    #message("input:   ${_input}")
    #message("copy:    ${copy}")
    #message("command: ${_command}")

    #file(MAKE_DIRECTORY ${_targetName})

    ADD_CUSTOM_COMMAND(OUTPUT  ${copy}
                       COMMAND ${CMAKE_COMMAND} -E copy  ${_input} ${copy}
                       DEPENDS ${_input})

    ADD_CUSTOM_COMMAND(OUTPUT ${_targetName}
                       COMMAND ${_command}
                       IMPLICIT_DEPENDS CXX ${_copy} ${_dependson})

    #GET_TARGET_PROPERTY(_sources ${_targetName} SOURCES)
    #FOREACH(_src ${_sources})
    #    SET_SOURCE_FILES_PROPERTIES(${_src} PROPERTIES OBJECT_DEPENDS ${_output})
    #ENDFOREACH()
    ##endif()

#    _PCH_GET_TARGET_COMPILE_FLAGS(_target_cflags ${_name} ${_output} ${_dowarn})
#
#	if(_target_c_flags)
#		SET_TARGET_PROPERTIES(${_targetName} 
#							  PROPERTIES	
#							  COMPILE_FLAGS ${_target_cflags} )
#	endif(_target_c_flags)
ENDMACRO(ADD_PRECOMPILED_HEADER)

