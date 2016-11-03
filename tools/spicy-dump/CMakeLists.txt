include_directories(${CMAKE_SOURCE_DIR}/spicy    ${CMAKE_BINARY_DIR}/spicy)
include_directories(${CMAKE_SOURCE_DIR}/libhilti ${CMAKE_BINARY_DIR}/libhilti)
include_directories(${CMAKE_SOURCE_DIR}/libspicy ${CMAKE_BINARY_DIR}/libspicy)
include_directories(${CMAKE_SOURCE_DIR}/util)

add_executable(spicy-dump spicy-dump.cc ascii.cc json.cc)
target_link_libraries(spicy-dump ${spicy_libs} ${hilti_libs})
set_target_properties(spicy-dump PROPERTIES COMPILE_DEFINITIONS "SPICY_DRIVER_JIT=1")
