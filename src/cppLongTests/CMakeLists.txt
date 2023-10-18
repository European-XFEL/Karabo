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
# Project that builds the C++ Karabo Framework Long Running Tests.
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
    "karabo-cpp-long-tests"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")
include("../cmake/resolve-karabo-lib-target.cmake")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
endif()

find_library(CPPUNIT cppunit)

resolveKaraboLibTarget()

# At this point either the Karabo Lib target was defined in a
# parent project or it has been found by find_package - the config
# package defines the Karabo Lib target as an imported one and
# assigns it the name "karabo".
# It is safe to go ahead and use the Karabo Lib target as a
# dependency of the test runners.

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testresults)

set(INTEGRATION_TEST_TARGETS
    devicesLongTestRunner
    xmsLongTestRunner
)

file(GLOB devicesLongTestRunner_FILES CONFIGURE_DEPENDS "devices/*.cc")
file(GLOB xmsLongTestRunner_FILES CONFIGURE_DEPENDS "xms/*.cc")

foreach(testTarget IN LISTS INTEGRATION_TEST_TARGETS)
    add_executable(${testTarget} ${${testTarget}_FILES})
    add_test(NAME ${testTarget} COMMAND ${testTarget})
    target_compile_options(
        ${testTarget}
        PRIVATE
            -DKARABO_TESTPATH=\"${CMAKE_CURRENT_SOURCE_DIR}/\"
    )

    target_link_libraries(
        ${testTarget}
        ${KARABO_LIB_TARGET_NAME}
        ${CPPUNIT}
    )
    message(STATUS "Added long running test executable ${testTarget}")

endforeach()
