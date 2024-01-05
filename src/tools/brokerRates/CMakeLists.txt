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
# Project that builds the Karabo Broker Rates Utility.

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-broker-rates"
    LANGUAGES C CXX
)

include("../../cmake/cxx-options.cmake")
include("../../cmake/karabo-lib-target-name.cmake")

set(BROKER_RATES_EXECUTABLE "karabo-brokerrates")

add_executable(${BROKER_RATES_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/brokerRates.cpp
)

target_link_libraries(
    ${BROKER_RATES_EXECUTABLE}
    ${KARABO_LIB_TARGET_NAME}
)

# $ORIGIN/../lib is needed by the loader to find 'libkarabo.so' in the install
# tree. $ORIGIN/../extern/lib is needed to find third-party dependencies like
# 'libboost_*.so'.
set_target_properties(
    ${BROKER_RATES_EXECUTABLE} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../extern/lib"
)

install(TARGETS ${BROKER_RATES_EXECUTABLE}
        RUNTIME DESTINATION bin
)
