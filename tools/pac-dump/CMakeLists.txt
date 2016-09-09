include_directories(${CMAKE_SOURCE_DIR}/binpac    ${CMAKE_BINARY_DIR}/binpac)
include_directories(${CMAKE_SOURCE_DIR}/libhilti  ${CMAKE_BINARY_DIR}/libhilti)
include_directories(${CMAKE_SOURCE_DIR}/libbinpac ${CMAKE_BINARY_DIR}/libbinpac)
include_directories(${CMAKE_SOURCE_DIR}/util)

add_executable(pac-dump pac-dump.cc ascii.cc json.cc)
target_link_libraries(pac-dump ${binpac_libs} ${hilti_libs})
set_target_properties(pac-dump PROPERTIES COMPILE_DEFINITIONS "PAC_DRIVER_JIT=1")
