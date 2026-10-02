# liburlparserConfig.cmake
#
# Usage (CMake >= 3.15):
#
#   find_package(liburlparser CONFIG REQUIRED)
#   target_link_libraries(app PRIVATE urlparser::urlparser)
#
# `urlparser::urlparser` is the same target you get from
# FetchContent/add_subdirectory. It carries everything a consumer needs: the
# compiled static library and the include directory for <urlparser.h>. All
# paths are resolved relative to this file, so the package works wherever it
# is unpacked (a wheel's site-packages, a system prefix, a vendored copy).
#
# For build systems that want plain paths instead of the target, these are
# also set (exact file name for the platform the package was built on):
#
#   urlparser_LIB_PATH       full path of the compiled library
#                            (liburlparser.a / urlparser.lib / ...)
#   urlparser_INCLUDE_PATH   directory containing urlparser.h
#
# When installed from the Python wheel, point CMake at it with:
#
#   python -m liburlparser --cmake-dir     # -> use as liburlparser_DIR

include("${CMAKE_CURRENT_LIST_DIR}/liburlparserTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/liburlparserPaths.cmake")   # generated at build time
