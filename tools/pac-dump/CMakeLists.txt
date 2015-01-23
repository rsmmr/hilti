
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../hilti ${CMAKE_CURRENT_BINARY_DIR}/../../hilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libhilti ${CMAKE_CURRENT_BINARY_DIR}/../../libhilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../binpac ${CMAKE_CURRENT_BINARY_DIR}/../../binpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libbinpac ${CMAKE_CURRENT_BINARY_DIR}/../../libbinpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../util)

find_package(PAPI)

if (${PAPI_FOUND} STREQUAL "TRUE" )
    set(PAPI "papi")
endif ()

add_executable(pac-dump pac-dump.cc ascii.cc json.cc)

set_target_properties(pac-dump PROPERTIES COMPILE_DEFINITIONS "PAC_DRIVER_JIT=1")

add_dependencies(pac-dump libhilti-rt libhilti-rt-dbg libbinpac-rt libbinpac-dbg)

target_link_libraries(pac-dump binpacxx ${HILTI_LIBS}
    binpac-rt-dbg
    ${LLVM_LIBS} ${LLVM_LDFLAGS}
    pcap ${PAPI} z
    )
