
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../hilti ${CMAKE_CURRENT_BINARY_DIR}/../../hilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libhilti ${CMAKE_CURRENT_BINARY_DIR}/../../libhilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../binpac ${CMAKE_CURRENT_BINARY_DIR}/../../binpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libbinpac ${CMAKE_CURRENT_BINARY_DIR}/../../libbinpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../util)

find_package(PAPI)

if (${PAPI_FOUND} STREQUAL "TRUE" )
    set(PAPI "papi")
endif ()

add_executable(pac-driver pac-driver.cc)

set_target_properties(pac-driver PROPERTIES COMPILE_DEFINITIONS "PAC_DRIVER_JIT=1")

add_dependencies(pac-driver libhilti-rt libbinpac-rt)

target_link_libraries(pac-driver binpacxx ${HILTI_LIBS}
    binpac-rt
    ${LLVM_LIBS} ${LLVM_LDFLAGS}
    pcap ${PAPI} z
    )
