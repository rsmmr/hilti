
set(hilti_build "${CMAKE_CURRENT_SOURCE_DIR}/../tools/hilti-build")

find_package(PAPI)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hilti  ${CMAKE_CURRENT_BINARY_DIR}/../hilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../binpac ${CMAKE_CURRENT_BINARY_DIR}/../binpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)

add_executable(hilti-config hilti-config.cc $<TARGET_OBJECTS:util>)
target_link_libraries(hilti-config ${LLVM_LIBS} ${LLVM_LDFLAGS}) # TODO: Should not be needed.

add_executable(binpac++ binpac++.cc)
target_link_libraries(binpac++ binpacxx hilti ${LLVM_LIBS} ${LLVM_LDFLAGS})

add_executable(hiltic hiltic.cc)
target_link_libraries(hiltic hilti ${LLVM_LIBS} ${LLVM_LDFLAGS})

add_executable(hilti-doc hilti-doc.cc)
target_link_libraries(hilti-doc binpacxx hilti ${LLVM_LIBS} ${LLVM_LDFLAGS})

add_subdirectory(pac-driver-jit)

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c hilti-rt-dbg hilti-rt
    COMMAND ${hilti_build}
    ARGS    -o ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c
    )

add_custom_target(hilti-prof ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof hiltic)

