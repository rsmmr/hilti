
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../hilti)

FindRequiredPackage(LLVM)

set(LIBS ast hilti util passes codegen)

add_executable(hiltic hiltic.cc)

target_link_libraries(hiltic ${LIBS} ${LLVM_LIBS} ${LLVM_LDFLAGS})

