# - Find libpcap. Requires at least 1.0
#
#  PCAP_CFLAGS      - CFLAGS for libpcap.
#  PCAP_LIBRARIES   - List of libraries when using zlib.
#  PCAP_FOUND       - True if libpcap is found.

if (PCAP_INCLUDE_DIR)
  set(PCAP_FOUND TRUE)
  
else (PCAP_INCLUDE_DIR)

  find_program(PCAP_CONFIG_EXECUTABLE  NAMES pcap-config PATHS)
  
  if (PCAP_CONFIG_EXECUTABLE)
      MESSAGE(STATUS "pcap-config found at: ${PCAP_CONFIG_EXECUTABLE}")
  else (PCAP_CONFIG_EXECUTABLE)
      MESSAGE(FATAL_ERROR "pcap-config is required, but not found! Do you have libpcap >= 1.0?")
  endif (PCAP_CONFIG_EXECUTABLE)

  exec_program(${PCAP_CONFIG_EXECUTABLE} ARGS --cflags OUTPUT_VARIABLE PCAP_CFLAGS )
  exec_program(${PCAP_CONFIG_EXECUTABLE} ARGS --libs   OUTPUT_VARIABLE PCAP_LIBRARIES )
  
  MESSAGE(STATUS "libpcap cflas: " ${PCAP_CFLAGS})
  MESSAGE(STATUS "libpcap libraries: " ${PCAP_LIBRARIES})
  
  set(PCAP_FOUND TRUE)
  
endif (PCAP_INCLUDE_DIR)
