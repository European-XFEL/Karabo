# Project that builds the Karabo Message Logger Utility.
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
# cmake -DCMAKE_PREFIX_PATH=$CONDA_PREFIX {{Karabo Framework Repository local working dir}}/src/brokerMessageLogger
#

cmake_minimum_required(VERSION 3.14)

project(
    "karabo-broker-message-logger"
    LANGUAGES C CXX
)

include("../../cmake/cxx-options.cmake")
include("../../cmake/karabo-lib-target-name.cmake")
include("../../cmake/resolve-karabo-lib-target.cmake")

resolveKaraboLibTarget()

# At this point either the Karabo Lib target was defined in a
# parent project or it has been setup as an imported target.
# It is safe to go ahead and use the Karabo Lib target as a
# dependency of the cppserver executable.

set(BROKER_LOGGER_EXECUTABLE "karabo-brokermessagelogger")

add_executable(${BROKER_LOGGER_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/brokerMessageLogger.cc)

target_link_libraries(
    ${BROKER_LOGGER_EXECUTABLE}
    ${KARABO_LIB_TARGET_NAME}
)

# $ORIGIN/../lib is needed by the loader to find 'libkarabo.so' in the install
# tree. $ORIGIN/../extern/lib is needed to find third-party dependencies like
# 'libboost_*.so'.
set_target_properties(
    ${BROKER_LOGGER_EXECUTABLE} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../extern/lib"
)

install(TARGETS ${BROKER_LOGGER_EXECUTABLE}
        RUNTIME DESTINATION bin
)
