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

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-cpp-long-tests"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
endif()

find_package(cppunit REQUIRED)

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
        cppunit::cppunit
    )
    message(STATUS "Added long running test executable ${testTarget}")

endforeach()
