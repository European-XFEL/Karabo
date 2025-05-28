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
# Project that builds the C++ Karabo Framework Integration Tests.

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-cpp-integration-tests"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
endif()

find_package(cppunit REQUIRED)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testresults)

add_library(INTEGRATION_RUNNER OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner.cc)
target_link_libraries(
    INTEGRATION_RUNNER
    ${KARABO_LIB_TARGET_NAME}
    cppunit::cppunit
)
add_library(TCP_ADAPTER OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/TcpAdapter.cc)
target_link_libraries(
    TCP_ADAPTER
    ${KARABO_LIB_TARGET_NAME}
    cppunit::cppunit
)


set(INTEGRATION_TEST_TARGETS
    dataLoggingIntegrTestRunner
    deviceIntegrTestRunner
    guiServerIntegrTestRunner
    pipelinedProcessingIntegrTestRunner
    propertyTestIntegrTestRunner
    sceneProviderIntegrTestRunner
    timingIntegrTestRunner
)


set(dataLoggingIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/BaseLogging_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/DataLogging_Test.cc
    $<TARGET_OBJECTS:INTEGRATION_RUNNER>
)

set(deviceIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Device_Test.cc
    $<TARGET_OBJECTS:INTEGRATION_RUNNER>
)


set(guiServerIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/GuiServer_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TestKaraboAuthServer.cc
    $<TARGET_OBJECTS:TCP_ADAPTER>
    $<TARGET_OBJECTS:INTEGRATION_RUNNER>
)

set(pipelinedProcessingIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/P2PSenderDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PipeReceiverDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PipelinedProcessing_Test.cc
    $<TARGET_OBJECTS:INTEGRATION_RUNNER>
)

set(propertyTestIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/PropertyTest_Test.cc
    $<TARGET_OBJECTS:INTEGRATION_RUNNER>
)


set(sceneProviderIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/NonSceneProviderTestDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneProviderTestDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneProvider_Test.cc
    $<TARGET_OBJECTS:TCP_ADAPTER>
    $<TARGET_OBJECTS:INTEGRATION_RUNNER>
)

set(timingIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/SimulatedTimeServerDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TimingTestDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Timing_Test.cc
    $<TARGET_OBJECTS:INTEGRATION_RUNNER>
)

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
    message(STATUS "Added integration test executable ${testTarget}")

endforeach()
