
include_directories(${CMAKE_SOURCE_DIR}/spicy    ${CMAKE_BINARY_DIR}/spicy)
include_directories(${CMAKE_SOURCE_DIR}/libhilti ${CMAKE_BINARY_DIR}/libhilti)
include_directories(${CMAKE_SOURCE_DIR}/libspicy ${CMAKE_BINARY_DIR}/libspicy)
include_directories(${CMAKE_SOURCE_DIR}/util)

add_executable(spicy-driver spicy-driver.cc)
target_link_libraries(spicy-driver ${spicy_libs} ${hilti_libs})
set_target_properties(spicy-driver PROPERTIES COMPILE_DEFINITIONS "SPICY_DRIVER_JIT=1")

