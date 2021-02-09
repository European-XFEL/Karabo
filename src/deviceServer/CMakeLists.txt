# Project that builds the Karabo C++ Device Server.
#
# Variables that influence the configuration behavior and that should be
# explicitly set at project configuration and generation time:

# CMAKE_PREFIX_PATH: when the target named "karabo" (the Karabo Shared Library target)
#                    is not known, this variable value should point to the root
#                    directory of an activated Conda environment. It can also have
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
# cmake -DCMAKE_PREFIX_PATH=$CONDA_PREFIX {{Karabo Framework Repository local working dir}}/src/deviceServer
#

cmake_minimum_required(VERSION 3.12)

project(
    "karabo-cpp-device-server"
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

set(DEVICE_SERVER_EXECUTABLE "karabo-cppserver")

add_executable(${DEVICE_SERVER_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/deviceServer.cc)

target_link_libraries(
    ${DEVICE_SERVER_EXECUTABLE}
    ${KARABO_LIB_TARGET_NAME}
)

install(TARGETS ${DEVICE_SERVER_EXECUTABLE}
        RUNTIME DESTINATION bin
)
