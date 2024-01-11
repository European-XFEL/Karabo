# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

cmake_minimum_required(VERSION 3.15)

project(
    "KarathonLib"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")

set(KARATHON_LIB_TARGET_NAME "karathon")

# Uses Python3_ROOT_DIR to avoid other Python interpreters that maybe on the
# system with wrong NumPy version or no NumPy package at all.
set(Python3_ROOT_DIR ${CMAKE_PREFIX_PATH})
find_package(
    Python3 3.11 EXACT REQUIRED
    COMPONENTS Interpreter Development NumPy
)

message(STATUS "Found NumPy version '${Python3_NumPy_VERSION}'.")
message(STATUS "NumPy include dirs = '${Python3_NumPy_INCLUDE_DIRS}'")

# From Boost 1.67 onwards, the identifier of the Boost Python component
# requires a suffix with the version of Python the Boost Python package has
# been built against. More details at:
# https://stackoverflow.com/questions/56067518/cmake-could-not-find-boost-python

find_package(Boost REQUIRED COMPONENTS python311)

file(GLOB_RECURSE KARATHON_SRCS CONFIGURE_DEPENDS "*.cc")

add_library(
    ${KARATHON_LIB_TARGET_NAME} SHARED
    ${KARATHON_SRCS}
)

# The last two RPATH entries are for when libkarathon.so is packaged as
# karathon.so inside INSTALL_PACKAGE_ROOT/extern/lib/python3.11/site-packages.
set_target_properties(
        ${KARATHON_LIB_TARGET_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN/../extern/lib;$ORIGIN;$ORIGIN/../../../../lib;$ORIGIN/../.."
)

# To avoid warnings about deprecated version for NumPy API.
target_compile_definitions(
    ${KARATHON_LIB_TARGET_NAME}
    PRIVATE
    NPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION
)

# Options lifted from the legacy Netbeans based project.
target_compile_options(
    ${KARATHON_LIB_TARGET_NAME}
    PRIVATE
    -Wfatal-errors
    -Wno-unused-local-typedefs
    -Wno-noexcept-type
    -Wall)

target_include_directories(
    ${KARATHON_LIB_TARGET_NAME}
    PUBLIC
    ${Python3_INCLUDE_DIRS}
    ${Python3_NumPy_INCLUDE_DIRS}
    # The Boost Python include directories are "inherited" from the
    # Karabo lib dependency - they are on the same tree as the other
    # components of Boost that libkarabo already used.
)

target_link_libraries(
    ${KARATHON_LIB_TARGET_NAME}
    ${KARABO_LIB_TARGET_NAME}
    boost::python311
    ${PYTHON_LIBRARIES}
)

install(
    TARGETS ${KARATHON_LIB_TARGET_NAME}
    LIBRARY DESTINATION "lib"
    PUBLIC_HEADER DESTINATION "include/karathon"
    RUNTIME DESTINATION "bin"
)

# Add karathon header files to the installation tree.
file(GLOB INCLUDE_FILES CONFIGURE_DEPENDS "*.hh")
foreach (FILE_PATH ${INCLUDE_FILES})
    install(FILES ${FILE_PATH} DESTINATION "include/karathon")
endforeach()
