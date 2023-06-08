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
# Project that builds the Karabo Message Logger Utility.
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
    "karabo-broker-message-logger"
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
