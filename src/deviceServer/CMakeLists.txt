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
# Project that builds the Karabo C++ Device Server.

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-cpp-device-server"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")

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
