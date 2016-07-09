
include(DefaultCompilerSettings)

set(hilti_build "${CMAKE_CURRENT_SOURCE_DIR}/../tools/hilti-build")

set(HILTI_LIBS            hilti ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt-dbg.bc ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt-native.a ${LLVM_LIBS} ${LLVM_LDFLAGS} pcap ${PAPI} ${PERFTOOLS})
set(HILTI_LIBS_NO_RUNTIME hilti ${LLVM_LIBS} ${LLVM_LDFLAGS} pcap ${PAPI} ${PERFTOOLS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hilti  ${CMAKE_CURRENT_BINARY_DIR}/../hilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../binpac ${CMAKE_CURRENT_BINARY_DIR}/../binpac)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)

add_executable(hilti-config hilti-config.cc $<TARGET_OBJECTS:util>)
target_link_libraries(hilti-config ${LLVM_LIBS} ${LLVM_LDFLAGS}) # TODO: Should not be needed.

# add_executable(binpac++ binpac++.cc)
# target_link_libraries(binpac++ binpacxx ${HILTI_LIBS})
# add_dependencies(binpac++ hilti-rt hilti-rt-dbg)

add_executable(hiltic hiltic.cc)
target_link_libraries(hiltic ${HILTI_LIBS})
add_dependencies(hiltic hilti-rt hilti-rt-dbg)

add_executable(hiltip hiltip.cc)
target_link_libraries(hiltip ${HILTI_LIBS_NO_RUNTIME})

add_executable(hilti-doc hilti-doc.cc)
# target_link_libraries(hilti-doc binpacxx ${HILTI_LIBS})
target_link_libraries(hilti-doc ${HILTI_LIBS})
add_dependencies(hilti-doc hilti-rt hilti-rt-dbg)

# add_subdirectory(pac-driver)
# add_subdirectory(pac-dump)

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c hilti-rt-dbg hilti-rt
    COMMAND ${hilti_build}
    ARGS    -o ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c
    )

# add_custom_target(hilti-prof ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof hiltic)

include(ShowCompilerSettings)
