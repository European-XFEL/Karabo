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
# The project that builds the C++ Karabo Framework Unit Tests.

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-cpp-unit-tests"
    LANGUAGES C CXX
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
endif()

include("../../cmake/cxx-options.cmake")
include("../../cmake/karabo-lib-target-name.cmake")

find_package(cppunit REQUIRED)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testresults)

# Some unit tests, more specifically io/H5File_test.cc, include header files
# using paths relative to the root level of the source tree. To support those
# without modifying any test source file, that directory is added in here.
set(KARABO_ROOT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Unit tests - under current directory, which is the root of the
# unit tests.
set(TEST_TARGETS
    coreTestRunner
    dataSchemaTestRunner
    dataTimeTestRunner
    dataTypesTestRunner
    dataIoTestRunner
    logTestRunner
    netTestRunner
    utilTestRunner
    xmsTestRunner
)

add_library(BROKER_UTILS OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/BrokerUtils.cc)
target_link_libraries(
    BROKER_UTILS
    ${KARABO_LIB_TARGET_NAME}
    cppunit::cppunit
)

add_library(WAIT_UTILS OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/WaitUtils.cc)
target_link_libraries(
    WAIT_UTILS
    ${KARABO_LIB_TARGET_NAME}
    cppunit::cppunit
)

add_library(TEST_RUNNER OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/testRunner.cc)
target_link_libraries(
    TEST_RUNNER
    ${KARABO_LIB_TARGET_NAME}
    cppunit::cppunit
)

add_library(TEST_RUNNER_NOLOOP OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/testRunnerNoLoop.cc)
target_link_libraries(
    TEST_RUNNER_NOLOOP
    ${KARABO_LIB_TARGET_NAME}
    cppunit::cppunit
)

set(coreTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/DeviceClient_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/core/InstanceChangeThrottler_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Runner_Test.cc
    $<TARGET_OBJECTS:WAIT_UTILS>
    $<TARGET_OBJECTS:TEST_RUNNER>
)

set(dataSchemaTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/data/schema/ConfigurationTestClasses.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/schema/Configurator_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/schema/Factory_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/schema/Schema_Test.cc  # includes some tests of type::Schema
    ${CMAKE_CURRENT_SOURCE_DIR}/data/schema/Validator_Test.cc
    $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>
)

set(dataTimeTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/data/time/DateTimeString_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/time/Epochstamp_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/time/TimeClasses_Test.cc
    $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>
)

set(dataTypesTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/AlarmCondition_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/Base64_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/ByteSwap_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/ClassInfo_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/Dims_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/Exception_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/HashFilter_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/Hash_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/NDArray_Test.cc
    # Schema is largely tested in ${CMAKE_CURRENT_SOURCE_DIR}/data/schema/Schema_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/Serializable_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/States_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/StringTools_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/types/Types_Test.cc
    $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>
)

set(dataIoTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/data/io/BufferSet_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/io/FileInputOutput_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/io/HashBinarySerializer_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/io/HashXmlSerializer_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/data/io/SchemaSerializer_Test.cc
    $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>
)

set(logTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/log/Logger_Test.cc
    $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>
)

set(netTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/net/Amqp_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/Broker_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/EventLoop_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/HttpClient_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/InfluxDbClient_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/MQTcpNetworking.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/NetworkInterface_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/ParseUrl_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/ReadAsyncStringUntil_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/Strand_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/net/TcpNetworking_Test.cc
    $<TARGET_OBJECTS:BROKER_UTILS>
    $<TARGET_OBJECTS:WAIT_UTILS>
    $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>
)

set(utilTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/util/DataLogUtils_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/util/MetaTools_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/util/Version_Test.cc
    $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>
)

set(xmsTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/xms/ImageData_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/xms/InputOutputChannel_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/xms/Memory_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/xms/Signal_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/xms/SignalSlotable_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/xms/Slot_Test.cc
    $<TARGET_OBJECTS:BROKER_UTILS>
    $<TARGET_OBJECTS:TEST_RUNNER>
)

foreach(testTarget IN LISTS TEST_TARGETS)
    add_executable(${testTarget} ${${testTarget}_FILES})
    add_test(NAME ${testTarget} COMMAND ${testTarget} testresults/${testTarget}.xml)
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
    message(STATUS "Added unit test executable ${testTarget}")
endforeach()

# Some io unit tests depend on example data files to succeed
# Here we copy them to the build directory.
add_custom_command(
    TARGET dataIoTestRunner
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/data/io/resources"
    "${CMAKE_CURRENT_BINARY_DIR}/data/io/resources"
)

#
# support for google test based Unit tests
find_package(GTest REQUIRED)

set(GTEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/core/testJsonToHashParsing.cc)

add_executable(GTestUnitTester ${GTEST_SOURCES})
target_link_libraries(GTestUnitTester
    PRIVATE
    GTest::gtest_main
    ${KARABO_LIB_TARGET_NAME})
include(GoogleTest)
gtest_discover_tests(GTestUnitTester)
