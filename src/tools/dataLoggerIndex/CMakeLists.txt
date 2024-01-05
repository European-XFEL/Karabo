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
# Project that builds the Karabo Idx Utilities for file based data logging.

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-data-logger-index"
    LANGUAGES C CXX
)

include("../../cmake/cxx-options.cmake")
include("../../cmake/karabo-lib-target-name.cmake")

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

# $ORIGIN/../lib is needed by the loader to find 'libkarabo.so' in the install
# tree. $ORIGIN/../extern/lib is needed to find third-party dependencies like
# 'libboost_*.so'.
set_target_properties(
    ${IDX_VIEW_EXECUTABLE} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../extern/lib"
)

install(TARGETS ${IDX_VIEW_EXECUTABLE}
        RUNTIME DESTINATION bin
)

# $ORIGIN/../lib is needed by the loader to find 'libkarabo.so' in the install
# tree. $ORIGIN/../extern/lib is needed to find third-party dependencies like
# 'libboost_*.so'.
set_target_properties(
    ${IDX_BUILD_EXECUTABLE} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../extern/lib"
)

install(TARGETS ${IDX_BUILD_EXECUTABLE}
        RUNTIME DESTINATION bin
)
