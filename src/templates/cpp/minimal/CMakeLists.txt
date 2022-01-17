# To disable building of tests, please set the cache variable BUILD_TESTS to
# 0 from the cmake configuration command-line. The default is to BUILD_TESTS.

cmake_minimum_required (VERSION 3.14)

project (__PACKAGE_NAME__ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)

# Creates file "compile_commands.json" in the build directory. It is useful
# for integrating with external tools such as "clangd". Some IDEs, like
# VSCode, also use information contained in this file to resolve pre-processor
# includes and provide better code completion suggestions.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT DEFINED ENV{KARABO} OR NOT EXISTS $ENV{KARABO})
  # KARABO env var not defined or invalid - try to infer the directory of a
  # Karabo installation.
  include("${CMAKE_SOURCE_DIR}/cmake/infer_karabo_env.cmake")
  inferKaraboEnv()
  if (DEFINED ENV{KARABO} AND EXISTS $ENV{KARABO})
      message(STATUS "Karabo installation found at '${KARABO_INST}'.")
      message(STATUS "It will be used during the CMake configuration phase.")
  else()
      message(
          FATAL_ERROR
          "KARABO environment variable not set and no Karabo installation found."
      )
  endif()
endif()

# Creates an IMPORTED target for the Karabo Framework Library and stores the
# target name in the variable KARABO_LIB_TARGET_NAME to be used downstream.
if (EXISTS "$ENV{KARABO}/lib/cmake/import_karabo_lib.cmake")
     # Prefer the importKaraboLib version shipped with the Karabo installation.
     include("$ENV{KARABO}/lib/cmake/import_karabo_lib.cmake")
else()
     # It may be an older Karabo installation that didn't come with the file.
     # Fallback to the version shipped with the device.
     include(cmake/import_karabo_lib.cmake)
endif()
importKaraboLib()

if(NOT DEFINED KARABO_LIB_TARGET_NAME)
    message(FATAL_ERROR
            "Failed to import Karabo Lib target - cannot proceed."
    )
endif()

# Builds tests if BUILD_TESTS is true (the default).
# The setting below is overridden by any previously set value (e.g. defined by
# the user in the command line).
set(BUILD_TESTS ON CACHE BOOL "Should build unit tests?")

add_subdirectory (src ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME})
