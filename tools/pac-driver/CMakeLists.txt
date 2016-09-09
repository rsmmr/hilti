
include_directories(${CMAKE_SOURCE_DIR}/binpac    ${CMAKE_BINARY_DIR}/binpac)
include_directories(${CMAKE_SOURCE_DIR}/libhilti  ${CMAKE_BINARY_DIR}/libhilti)
include_directories(${CMAKE_SOURCE_DIR}/libbinpac ${CMAKE_BINARY_DIR}/libbinpac)
include_directories(${CMAKE_SOURCE_DIR}/util)

add_executable(pac-driver pac-driver.cc)
target_link_libraries(pac-driver ${binpac_libs} ${hilti_libs})
set_target_properties(pac-driver PROPERTIES COMPILE_DEFINITIONS "PAC_DRIVER_JIT=1")

