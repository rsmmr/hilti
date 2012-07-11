
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hilti)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../hilti)

FindRequiredPackage(LLVM)

set(LIBS hilti hilti-ffi)

add_executable(hiltic hiltic.cc)
add_executable(hilti-doc hilti-doc.cc)

target_link_libraries(hiltic ${LIBS} ${LLVM_LIBS} ${LLVM_LDFLAGS})
target_link_libraries(hilti-doc ${LIBS} ${LLVM_LIBS} ${LLVM_LDFLAGS})

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../hilti-build/hilti-build
    ARGS    -m -o ${CMAKE_CURRENT_BINARY_DIR}/hilti-prof ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c
    )

#add_custom_target(hilti-prof ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hilti-prof.c)

