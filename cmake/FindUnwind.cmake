# Try to find libunwind headers and libraries.
#
# Usage of this module as follows:
#
#     find_package(Unwind)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Unwind_PREFIX         Set this variable to the root installation of
#                      libUnwind if the module has problems finding the
#                      proper installation path.
#
# Variables defined by this module:
#
#  Unwind_FOUND              System has Unwind libraries and headers
#  Unwind_LIBRARIES          The Unwind library
#  Unwind_INCLUDE_DIRS       The location of Unwind headers

find_path(Unwind_PREFIX
    NAMES include/libunwind.h
)

find_library(Unwind_LIBRARIES
    # Pick the static library first for easier run-time linking.
    NAMES libunwind
    HINTS ${Unwind_PREFIX}/lib
)

find_path(Unwind_INCLUDE_DIRS
    NAMES libunwind.h
    HINTS ${Unwind_PREFIX}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Unwind DEFAULT_MSG
    Unwind_LIBRARIES
    Unwind_INCLUDE_DIRS
)

mark_as_advanced(
    Unwind_PREFIX_DIRS
    Unwind_LIBRARIES
    Unwind_INCLUDE_DIRS
)
