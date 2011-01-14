# Detect LLVM and set various variable to link against the different component of LLVM
#
# [Robin] NOTE: This is adapted from http://code.roadsend.com/pcc/export/796/trunk/rphp/cmake/modules/FindLLVM.cmake
# [Robin] We also search for clang.
# [Robin] Removed messages outputting the variables; added that to EnableLLVM.cmake instead.
#
# NOTE: This is a modified version of the module originally found in the OpenGTL project
# at www.opengtl.org
#
# LLVM_BIN_DIR : directory with LLVM binaries
# LLVM_LIB_DIR : directory with LLVM library
# LLVM_INCLUDE_DIR : directory with LLVM include
#
# LLVM_C_COMPILE_FLAGS : compile flags needed to build a C program using LLVM headers
# LLVM_CXX_COMPILE_FLAGS : compile flags needed to build a C++ program using LLVM headers
# LLVM_LDFLAGS : ldflags needed to link
# LLVM_LIBS_CORE : ldflags needed to link against a LLVM core library
# LLVM_LIBS_JIT : ldflags needed to link against a LLVM JIT
# LLVM_LIBS_JIT_OBJECTS : objects you need to add to your source when using LLVM JIT

if (LLVM_INCLUDE_DIR)
  set(LLVM_FOUND TRUE)
else (LLVM_INCLUDE_DIR)

  find_program(LLVM_CONFIG_EXECUTABLE
      NAMES llvm-config
      PATHS
      /opt/local/bin
  )

  find_program(LLVM_GXX_EXECUTABLE
      NAMES llvm-g++ llvmg++
      PATHS
      /opt/local/bin
  )

  find_program(LLVM_CLANG_EXECUTABLE
      NAMES clang
      PATHS
      /opt/local/bin
  )

  if (LLVM_GXX_EXECUTABLE)
      MESSAGE(STATUS "Found llvm-g++ t: ${LLVM_GXX_EXECUTABLE}")
#  else(LLVM_GXX_EXECUTABLE)
#      MESSAGE(FATAL_ERROR "llvm-g++ is required, but not found!")
  endif(LLVM_GXX_EXECUTABLE)

  if (LLVM_CLANG_EXECUTABLE)
      MESSAGE(STATUS "Found clang at: ${LLVM_CLANG_EXECUTABLE}")
#      set(CMAKE_C_COMPILER "clang")
  endif(LLVM_CLANG_EXECUTABLE)

  MACRO(FIND_LLVM_LIBS LLVM_CONFIG_EXECUTABLE _libname_ LIB_VAR OBJECT_VAR)
    exec_program( ${LLVM_CONFIG_EXECUTABLE} ARGS --libs ${_libname_}  OUTPUT_VARIABLE ${LIB_VAR} )
    STRING(REGEX MATCHALL "[^ ]*[.]o[ $]"  ${OBJECT_VAR} ${${LIB_VAR}})
    SEPARATE_ARGUMENTS(${OBJECT_VAR})
    STRING(REGEX REPLACE "[^ ]*[.]o[ $]" ""  ${LIB_VAR} ${${LIB_VAR}})
  ENDMACRO(FIND_LLVM_LIBS)


  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --bindir OUTPUT_VARIABLE LLVM_BIN_DIR )
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libdir OUTPUT_VARIABLE LLVM_LIB_DIR )
  #MESSAGE(STATUS "LLVM lib dir: " ${LLVM_LIB_DIR})
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR )


  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --cxxflags OUTPUT_VARIABLE LLVM_CXX_COMPILE_FLAGS )
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --cflags OUTPUT_VARIABLE LLVM_C_COMPILE_FLAGS )
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --ldflags   OUTPUT_VARIABLE LLVM_LDFLAGS )
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libs core bitreader bitwriter linker OUTPUT_VARIABLE LLVM_LIBS_CORE )
  FIND_LLVM_LIBS( ${LLVM_CONFIG_EXECUTABLE} "jit native" LLVM_LIBS_JIT LLVM_LIBS_JIT_OBJECTS )
  #STRING(REPLACE " -lLLVMCore -lLLVMSupport -lLLVMSystem" "" LLVM_LIBS_JIT ${LLVM_LIBS_JIT_RAW})

  if(LLVM_INCLUDE_DIR)
    set(LLVM_FOUND TRUE)
  endif(LLVM_INCLUDE_DIR)

  if(LLVM_FOUND)
    message(STATUS "Found LLVM include directory: ${LLVM_INCLUDE_DIR}")
  else(LLVM_FOUND)
    if(LLVM_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find LLVM")
    endif(LLVM_FIND_REQUIRED)
  endif(LLVM_FOUND)

endif (LLVM_INCLUDE_DIR)
