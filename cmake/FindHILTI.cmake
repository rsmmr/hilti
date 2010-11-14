# Detect HILTI and set various variables.
#
# HILTI_CFLAGS    : compile flags needed to build a C program using HILTI headers.
# HILTI_LDFLAGS   : ldflags needed to link

find_program(HILTI_CONFIG_EXECUTABLE
      NAMES hilti-config
      PATHS
      /opt/local/bin ../../hilti/tools/hilti-config
  )

exec_program(${HILTI_CONFIG_EXECUTABLE} ARGS --cflags  OUTPUT_VARIABLE HILTI_CFLAGS)
exec_program(${HILTI_CONFIG_EXECUTABLE} ARGS --ldflags OUTPUT_VARIABLE HILTI_LDFLAGS)

if(HILTI_CONFIG_EXECUTABLE)
    set(HILTI_FOUND TRUE)
endif(HILTI_CONFIG_EXECUTABLE)

if(HILTI_FOUND)
    message(STATUS "Found hilti-config: ${HILTI_CONFIG_EXECUTABLE}")
else(HILTI_FOUND)
    message(FATAL_ERROR "Could NOT find hilti-config")
endif(HILTI_FOUND)

