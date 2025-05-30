# This file is intended to be used together with Karabo:
#
# http://www.karabo.eu
#
# IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
# Karabo itself is licensed under the terms of the MPL 2.0 license.
# To disable building of tests, please set the cache variable BUILD_TESTS to
# 0 from the cmake configuration command-line. The default is to BUILD_TESTS.

cmake_minimum_required (VERSION 3.14)

project (__PACKAGE_NAME__ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Creates file "compile_commands.json" in the build directory. It is useful
# for integrating with external tools such as "clangd". Some IDEs, like
# VSCode, also use information contained in this file to resolve pre-processor
# includes and provide better code completion suggestions.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT DEFINED ENV{KARABO} OR NOT EXISTS $ENV{KARABO})
  message(
      FATAL_ERROR
      "KARABO environment variable not set and no Karabo installation found."
  )
endif()

find_package(PkgConfig)
set(PKG_CONFIG_ARGN "--define-variable=KARABO=$ENV{KARABO}")
set(ENV{PKG_CONFIG_PATH} "$ENV{KARABO}/lib:$ENV{PKG_CONFIG_PATH}")
pkg_check_modules(karabo IMPORTED_TARGET karaboDependencies>=1.0)

# if karaboDependencies.pc is found in the Karabo installation
# pkg_check_modules() creates an CMake imported target for it
set(KARABO_LIB_TARGET_NAME "PkgConfig::karabo")

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
