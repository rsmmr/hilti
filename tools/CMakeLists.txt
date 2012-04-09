
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hilti)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../hilti)

FindRequiredPackage(LLVM)

set(LIBS hilti hilti-ffi)

add_executable(hiltic hiltic.cc)

target_link_libraries(hiltic ${LIBS} ${LLVM_LIBS} ${LLVM_LDFLAGS})

