
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../hilti ${CMAKE_CURRENT_BINARY_DIR}/../../hilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libhilti ${CMAKE_CURRENT_BINARY_DIR}/../../libhilti)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../binpac ${CMAKE_CURRENT_BINARY_DIR}/../../binpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libbinpac ${CMAKE_CURRENT_BINARY_DIR}/../../libbinpac)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../util)

find_package(PAPI)

if (${PAPI_FOUND} STREQUAL "TRUE" )
    set(PAPI "papi")
endif ()

add_executable(pac-driver-jit pac-driver-jit.cc)

add_dependencies(pac-driver-jit
    ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt.bc
    ${PROJECT_BINARY_DIR}/libbinpac/libbinpac-rt.bc
)

target_link_libraries(pac-driver-jit binpacxx hilti ${LLVM_LIBS} ${LLVM_LDFLAGS}
    ${PROJECT_BINARY_DIR}/libhilti/libhilti-rt.bc
    ${PROJECT_BINARY_DIR}/libbinpac/libbinpac-rt.bc
    pcap ${PAPI}
    )
