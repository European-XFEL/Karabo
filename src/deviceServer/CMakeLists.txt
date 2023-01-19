# Project that builds the Karabo C++ Device Server.
#
# Variables that influence the configuration behavior and that should be
# explicitly set at project configuration and generation time:
#
# CMAKE_PREFIX_PATH: when the target named "karabo" (the Karabo Shared Library 
#                    target) is not known, this variable value should have the
#                    root directory of the external build (and run) time 
#                    dependencies of the karabo shared library.
#

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-cpp-device-server"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")
include("../cmake/resolve-karabo-lib-target.cmake")

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

# $ORIGIN/../lib is needed by the loader to find 'libkarabo.so' in the install
# tree. $ORIGIN/../extern/lib is needed to find third-party dependencies like
# 'libboost_*.so'.
set_target_properties(
    ${DEVICE_SERVER_EXECUTABLE} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../extern/lib"
)

# karabo-cppserver requires $KARABO/var/data to run. In here, we create
# that directory in the location the $KARABO installation dir will be resolved
# to in the CMake build tree. This is required to run karabo-deviceServer from
# the device tree.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../var/data)
# Also makes sure that the folder exists in the install location
install(DIRECTORY DESTINATION "var/data")

# karabo-cppserver requires a path to load device libraries from. This path
# defaults to $KARABO/plugins and, as this path is usually not explicitly
# set, in here we create that default path in both the build and the installation
# trees.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../plugins)
install(DIRECTORY DESTINATION "plugins")

install(TARGETS ${DEVICE_SERVER_EXECUTABLE}
        RUNTIME DESTINATION bin
)
