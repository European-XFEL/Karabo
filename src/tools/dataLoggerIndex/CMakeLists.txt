# Project that builds the Karabo Idx Utilities for file based data logging.
#
# Variables that influence the configuration behavior and that should be
# explicitly set at project configuration and generation time:
#
# CMAKE_PREFIX_PATH: when the target named "karabo" (the Karabo Shared Library target)
#                    is not known, this variable value should point to the root
#                    directory of an activated Conda environment. It can also have the
#                    default value as initialized by CMAKE for a Docker Image or for a
#                    Physical System (for Ubuntus this is "/usr/local"). In all three
#                    cases, namely Conda, Docker Image or Physical System, the build
#                    dependencies of the Karabo Framework lib as well as the Karabo
#                    Framework shared lib itself must be available for this script
#                    to succeed.
#
# Example of command line to configure and generate the build for this project inside
# an activated Conda environment with the Karabo Framework lib package (and its dependencies)
# installed:
#
# cmake -DCMAKE_PREFIX_PATH=$CONDA_PREFIX {{Karabo Framework Repository local working dir}}/src/tools/dataLoggerIndex
#

cmake_minimum_required(VERSION 3.12)

project(
    "karabo-data-logger-index"
    LANGUAGES C CXX
)

include("../../../cmake/cxx-options.cmake")
include("../../../cmake/karabo-lib-target-name.cmake")
include("../../../cmake/resolve-karabo-lib-target.cmake")

resolveKaraboLibTarget()

# At this point either the Karabo Lib target was defined in a
# parent project or it has been setup as an imported target.
# It is safe to go ahead and use the Karabo Lib target as a
# dependency of the cppserver executable.

set(IDX_VIEW_EXECUTABLE "karabo-idxview")

add_executable(${IDX_VIEW_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/idxView.cc
)

target_link_libraries(
    ${IDX_VIEW_EXECUTABLE}
    ${KARABO_LIB_TARGET_NAME}
)

set(IDX_BUILD_EXECUTABLE "karabo-idxbuild")

add_executable(${IDX_BUILD_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/idxBuild.cc
)

target_link_libraries(
    ${IDX_BUILD_EXECUTABLE}
    ${KARABO_LIB_TARGET_NAME}
)

# The second value for RPATH is needed to find the libs
# that libkarabo itself links against (like libboost)
# when libkarabo and the device server executable
# are installed on a tree that's not the same as the
# tree that hosts the libkarabo dependencies.
# This can happen while developing the Framework itself,
# when the developer can have an environment with
# the libkarabo dependencies installed without having the
# Framework as an installed package.
set_target_properties(
    ${IDX_VIEW_EXECUTABLE} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;${CMAKE_PREFIX_PATH}/lib"
)

install(TARGETS ${IDX_VIEW_EXECUTABLE}
        RUNTIME DESTINATION bin
)

set_target_properties(
    ${IDX_BUILD_EXECUTABLE} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;${CMAKE_PREFIX_PATH}/lib"
)

install(TARGETS ${IDX_BUILD_EXECUTABLE}
        RUNTIME DESTINATION bin
)
