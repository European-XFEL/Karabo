# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
cmake_minimum_required(VERSION 3.15)

project(
    "KarathonLib"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")
include("../cmake/resolve-karabo-lib-target.cmake")

resolveKaraboLibTarget()

set(KARATHON_LIB_TARGET_NAME "karathon")

# Uses Python3_ROOT_DIR to avoid other Python interpreters that maybe on the
# system with wrong NumPy version or no NumPy package at all.
set(Python3_ROOT_DIR ${CMAKE_PREFIX_PATH})
find_package(
    Python3 3.8 EXACT REQUIRED
    COMPONENTS Interpreter Development NumPy
)

message(STATUS "Found NumPy version '${Python3_NumPy_VERSION}'.")
message(STATUS "NumPy include dirs = '${Python3_NumPy_INCLUDE_DIRS}'")

# From Boost 1.67 onwards, the identifier of the Boost Python component
# requires a suffix with the version of Python the Boost Python package has
# been built against. More details at:
# https://stackoverflow.com/questions/56067518/cmake-could-not-find-boost-python
#
# Force cmake to ignore any BoostConfig.cmake and boost-config.cmake that can
# be found on the local system when it already has Boost installed and Boost has
# version 1.70+ or has been compiled with the boost-cmake project
# (details at https://cmake.org/cmake/help/latest/module/FindBoost.html).
# For this project, we always want to find the Boost installation in the
# CMAKE_PREFIX_PATH tree.
set(Boost_NO_BOOST_CMAKE 1)
find_package(Boost 1.68 EXACT REQUIRED COMPONENTS python38)

file(GLOB_RECURSE KARATHON_SRCS CONFIGURE_DEPENDS "*.cc")

add_library(
    ${KARATHON_LIB_TARGET_NAME} SHARED
    ${KARATHON_SRCS}
)

# The last two RPATH entries are for when libkarathon.so is packaged as
# karathon.so inside INSTALL_PACKAGE_ROOT/extern/lib/python3.8/site-packages.
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
    Boost::python38
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
