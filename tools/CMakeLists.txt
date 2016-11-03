
include(DefaultCompilerSettings)

set(hilti_build "${CMAKE_CURRENT_SOURCE_DIR}/../tools/hilti-build")

if ( "${CMAKE_BUILD_TYPE}" MATCHES "Debug" )
    set(hilti_runtime hilti-rt-dbg)
    set(spicy_runtime spicy-rt-dbg)
else ()
    set(hilti_runtime hilti-rt)
    set(spicy_runtime spicy-rt)
endif ()

set(hilti_libs            hilti ${hilti_runtime})
set(hilti_libs_no_runtime hilti)
set(spicy_libs            spicy ${spicy_runtime})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hilti  ${CMAKE_CURRENT_BINARY_DIR}/../hilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../spicy  ${CMAKE_CURRENT_BINARY_DIR}/../spicy)

add_executable(hilti-config hilti-config.cc $<TARGET_OBJECTS:util>)
target_link_libraries(hilti-config ${LLVM_LIBS} ${LLVM_LDFLAGS}) # TODO: Should not be needed.

add_executable(spicy-config spicy-config.cc $<TARGET_OBJECTS:util>)

add_executable(spicyc spicyc.cc)
target_link_libraries(spicyc spicy ${hilti_libs})

add_executable(hiltic hiltic.cc)
add_dependencies(hiltic hilti-rt hilti-rt-dbg)
target_link_libraries(hiltic ${hilti_libs})

add_executable(hiltip hiltip.cc)
target_link_libraries(hiltip hilti)

add_executable(hilti-doc hilti-doc.cc)
target_link_libraries(hilti-doc ${hilti_libs})

add_executable(spicy-doc spicy-doc.cc)
target_link_libraries(spicy-doc spicy ${hilti_libs})

add_subdirectory(spicy-driver)
add_subdirectory(spicy-dump)

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c hilti-rt-dbg hilti-rt
    COMMAND ${hilti_build}
    ARGS    -o ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c
    )

# add_custom_target(hilti-prof ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof hiltic)

include(ShowCompilerSettings)
