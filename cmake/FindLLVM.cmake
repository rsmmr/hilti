# Detect LLVM and set various variable to link against the different component of LLVM
#
# [Robin] NOTE: This is adapted from http://code.roadsend.com/pcc/export/796/trunk/rphp/cmake/modules/FindLLVM.cmake
# [Robin] We also search for clang.
# [Robin] Removed messages outputting the variables; added that to EnableLLVM.cmake instead.
# [Robin] Removed llvm-gcc/g++ stuff.
# [Robin] General cleanup, and renamed some of the output variables.
# [Robin] Added LLVM_TRIPLE.
# [Robin] Added COMPILER_RT_LIB_DIR.
#
# NOTE: This is a modified version of the module originally found in the OpenGTL project
# at www.opengtl.org
#
# LLVM_FOUND        TRUE if LLVM and clang were found.
#
# LLVM_BIN_DIR      Directory with LLVM binaries.
# LLVM_LIB_DIR      Directory with LLVM library.
# LLVM_INCLUDE_DIR  Directory with LLVM include.
#
# LLVM_CFLAGS       Compiler flags needed to build a C program using LLVM headers.
# LLVM_CXXFLAGS     Compiler flags needed to build a C++ program using LLVM headers.
# LLVM_LDFLAGS      Linker flags for linking with the core LLVM libraries.
# LLVM_LIBS         Linker libraries for linking with the core LLVM libraries.
#
# LLVM_{CFLAGS,CXXFLAGS,LDFLAGS,LIBS}_AS_STRING Same, but as space-separated strings instead of lists.
#
# LLVM_CONFIG_EXEC  Path of the llvm-config executable.
# LLVM_CLANG_EXEC   Path of the clang executable.
# LLVM_CLANGXX_EXEC Path of the clang++ executable.
#
# LLVM_TRIPLE       Triple used to configure LLVM.
# LLVM_TRIPLE_CLANG Triple that clang reports to use by default (not necessarily the same as LLVM_TRIPLE).
# LLVM_DATA_LAYOUT_CLANG Data layout that clang uses by default.
#
# COMPILER_RT_LIB_DIR Directory where the compiler-rt runtime
#                     libraries are installed; empty if not found.

#
# [Disabled for now. -Robin] LLVM_LIBS_JIT : ldflags needed to link against a LLVM JIT
# [Disabled for now. -Robin] LLVM_LIBS_JIT_OBJECTS : objects you need to add to your source when using LLVM JIT

  set(CLANG_PLATFORM_SPECS ${CMAKE_SOURCE_DIR}/scripts/clang-platform-specs)

  find_program(LLVM_CONFIG_EXEC
      NAMES llvm-config
      PATHS /opt/local/bin
  )

  find_program(LLVM_CLANG_EXEC
      NAMES clang
      PATHS /opt/local/bin
  )

  find_program(LLVM_CLANGXX_EXEC
      NAMES clang++
      PATHS /opt/local/bin
  )

  set(LLVM_FOUND TRUE)

  if ( LLVM_CONFIG_EXEC )
      MESSAGE(STATUS "Found llvm-config at ${LLVM_CONFIG_EXEC}")
  else ()
      MESSAGE(STATUS "llvm-config not found")
      set(LLVM_FOUND FALSE)
  endif ()

  if ( LLVM_CLANG_EXEC )
      MESSAGE(STATUS "Found clang at ${LLVM_CLANG_EXEC}")
  else ()
      MESSAGE(STATUS "clang not found")
      set(LLVM_FOUND FALSE)
  endif ()

  if ( LLVM_CLANGXX_EXEC )
      MESSAGE(STATUS "Found clang++ at ${LLVM_CLANGXX_EXEC}")
  else ()
      MESSAGE(STATUS "clang++ not found")
      set(LLVM_FOUND FALSE)
  endif ()

  MACRO(FIND_LLVM_LIBS LLVM_CONFIG_EXEC _libname_ LIB_VAR OBJECT_VAR)
    exec_program( ${LLVM_CONFIG_EXEC} ARGS --libs ${_libname_}  OUTPUT_VARIABLE ${LIB_VAR} )
    STRING(REGEX MATCHALL "[^ ]*[.]o[ $]"  ${OBJECT_VAR} ${${LIB_VAR}})
    SEPARATE_ARGUMENTS(${OBJECT_VAR})
    STRING(REGEX REPLACE "[^ ]*[.]o[ $]" ""  ${LIB_VAR} ${${LIB_VAR}})
  ENDMACRO(FIND_LLVM_LIBS)

  exec_program(${LLVM_CONFIG_EXEC} ARGS --bindir     OUTPUT_VARIABLE LLVM_BIN_DIR)
  exec_program(${LLVM_CONFIG_EXEC} ARGS --libdir     OUTPUT_VARIABLE LLVM_LIB_DIR)
  exec_program(${LLVM_CONFIG_EXEC} ARGS --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR)

  exec_program(${LLVM_CONFIG_EXEC} ARGS --cflags     OUTPUT_VARIABLE LLVM_CFLAGS )
  exec_program(${LLVM_CONFIG_EXEC} ARGS --cxxflags   OUTPUT_VARIABLE LLVM_CXXFLAGS )
  exec_program(${LLVM_CONFIG_EXEC} ARGS --ldflags    OUTPUT_VARIABLE LLVM_LDFLAGS )

  exec_program(${LLVM_CONFIG_EXEC} ARGS --host-target OUTPUT_VARIABLE LLVM_TRIPLE)
  exec_program(${LLVM_CONFIG_EXEC} ARGS --prefix      OUTPUT_VARIABLE LLVM_PREFIX)

  exec_program(${CLANG_PLATFORM_SPECS} ARGS --triple      OUTPUT_VARIABLE LLVM_TRIPLE_CLANG)
  exec_program(${CLANG_PLATFORM_SPECS} ARGS --data-layout OUTPUT_VARIABLE LLVM_DATA_LAYOUT_CLANG)

  # llvm-config includes stuff we don't want.
  set(cflags_to_remove "-fno-exceptions" "-O." "-fomit-frame-pointer" "-stdlib=libc\\+\\+" "-D_DEBUG")

  foreach(f ${cflags_to_remove})
      STRING(REGEX REPLACE "${f}" "" LLVM_CFLAGS   "${LLVM_CFLAGS}")
      STRING(REGEX REPLACE "${f}" "" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
  endforeach()

  # This needs oprofile support in LLVM: oprofilejit
  set(llvm_libs "object core bitreader bitwriter linker asmparser interpreter executionengine jit mcjit runtimedyld nativecodegen ipo irreader x86asmparser")
  exec_program(${LLVM_CONFIG_EXEC} ARGS --libs        OUTPUT_VARIABLE LLVM_LIBS)

  # --system-libs exists since LLVM 3.5.
  execute_process(COMMAND ${LLVM_CONFIG_EXEC} --system-libs OUTPUT_VARIABLE LLVM_SYSTEM_LIBS ERROR_QUIET)
  set(LLVM_LIBS "${LLVM_LIBS} ${LLVM_SYSTEM_LIBS}")

  set(LLVM_LDFLAGS "${LLVM_LDFLAGS} -Wl,-rpath,${LLVM_LIB_DIR}")

  string(STRIP "${LLVM_LDFLAGS}" LLVM_LDFLAGS)
  string(STRIP "${LLVM_LIBS}" LLVM_LIBS)

  separate_arguments(LLVM_LDFLAGS)
  separate_arguments(LLVM_CFLAGS)
  separate_arguments(LLVM_CXXFLAGS)
  separate_arguments(LLVM_LIBS)

  string(REGEX REPLACE ";+" " " LLVM_LDFLAGS_AS_STRING "${LLVM_LDFLAGS}")
  string(REGEX REPLACE ";+" " " LLVM_CFLAGS_AS_STRING "${LLVM_CFLAGS}")
  string(REGEX REPLACE ";+" " " LLVM_CXXFLAGS_AS_STRING "${LLVM_CXXFLAGS}")
  string(REGEX REPLACE ";+" " " LLVM_LIBS_AS_STRING "${LLVM_LIBS}")

  # FIND_LLVM_LIBS( ${LLVM_CONFIG_EXEC} "jit native" LLVM_LIBS_JIT LLVM_LIBS_JIT_OBJECTS )
  # STRING(REPLACE " -lLLVMCore -lLLVMSupport -lLLVMSystem" "" LLVM_LIBS_JIT ${LLVM_LIBS_JIT_RAW})

  # Determine path to compiler-rt runtime libraries.
  exec_program(${LLVM_CLANG_EXEC} ARGS --version OUTPUT_VARIABLE CLANG_VERSION)
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" CLANG_VERSION "${CLANG_VERSION}")

  if ( NOT CLANG_VERSION )
      string(REGEX MATCH "[0-9]+\\.[0-9]+" CLANG_VERSION "${CLANG_VERSION}.0")
  endif ()

  exec_program("uname -s" OUTPUT_VARIABLE os)
  string(TOLOWER "${os}" os)
  set(COMPILER_RT_LIB_DIR ${LLVM_PREFIX}/lib/clang/${CLANG_VERSION}/lib/${os})

  if ( EXISTS "${COMPILER_RT_LIB_DIR}" )
      MESSAGE(STATUS "Compiler-rt runtime directory: ${COMPILER_RT_LIB_DIR}")
  else ()
      MESSAGE(STATUS "Cannot find compiler-rt runtime directory (tried ${COMPILER_RT_LIB_DIR})")
      set(COMPILER_RT_LIB_DIR)
  endif ()
