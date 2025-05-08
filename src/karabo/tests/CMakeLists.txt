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
find_package(pugixml REQUIRED)
find_package(spdlog REQUIRED)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testresults)

# Unit tests that don't link to libkarabo.so

# Framework object file sets used by the statically linked unit tests
file(GLOB karaboLog_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/../log/*.cc")

file(GLOB karaboUtil_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/../util/*.cc")

file(GLOB dataTypes_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/../data/types/*.cc")

file(GLOB dataSchema_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/../data/schema/*.cc")

file(GLOB dataTime_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/../data/time/*.cc")

file(GLOB dataIo_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/../data/io/*.cc")

# NOTE: The dataSchema tests depend on some types from karabo::xms due to a
# reference to karabo::xms::ImageData from Schema_Test::testImageElement and a
# reference to karabo::xms::OutputChannel from
# Validator_Test::testOutputChannelSchemaRemoval.
# For now, dataSchemaTestRunner is kept linking to libkarabo.so.

# Runner object file set for the statically linked unit tests
add_library(TEST_RUNNER_NOLOOP OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/testRunnerNoLoop.cc)
target_link_libraries(
    TEST_RUNNER_NOLOOP
    cppunit::cppunit
)

file(GLOB dataTypesTestRunner_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/data/types/*.cc")
list(APPEND dataTypesTestRunner_FILES $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>)

add_executable(dataTypesTestRunner ${dataTypes_FILES} ${dataSchema_FILES} ${dataTime_FILES} ${karaboLog_FILES} ${dataTypesTestRunner_FILES})
target_compile_options(dataTypesTestRunner PRIVATE -DKARABO_TESTPATH=\"${CMAKE_CURRENT_SOURCE_DIR}/\")
target_link_libraries(dataTypesTestRunner spdlog::spdlog cppunit::cppunit)
target_include_directories(dataTypesTestRunner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
add_test(NAME dataTypesTestRunner COMMAND dataTypesTestRunner testresults/dataTypeTestRunner.xml)

file(GLOB dataTimeTestRunner_FILES CMAKE_CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/data/time/*.cc")
list(APPEND dataTimeTestRunner_FILES $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>)

add_executable(dataTimeTestRunner ${dataTime_FILES} ${dataTypes_FILES} ${dataSchema_FILES} ${karaboUtil_FILES} ${karaboLog_FILES} ${dataTimeTestRunner_FILES})
target_compile_options(dataTimeTestRunner PRIVATE -DKARABO_TESTPATH=\"${CMAKE_CURRENT_SOURCE_DIR}/\")
target_link_libraries(dataTimeTestRunner spdlog::spdlog cppunit::cppunit)
target_include_directories(dataTimeTestRunner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
add_test(NAME dataTimeTestRunner COMMAND dataTimeTestRunner testresults/dataTimeTestRunner.xml)

file(GLOB dataIoTestRunner_FILES CMAKE_CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/data/io/*.cc")
list(APPEND dataIoTestRunner_FILES $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>)

add_executable(dataIoTestRunner ${dataIo_FILES} ${dataTypes_FILES} ${dataSchema_FILES} ${dataTime_FILES} ${karaboUtil_FILES} ${karaboLog_FILES} ${dataIoTestRunner_FILES})
target_compile_options(dataIoTestRunner PRIVATE -DKARABO_TESTPATH=\"${CMAKE_CURRENT_SOURCE_DIR}/\")
# NOTE: defining _SO__ even though we are statically linking to Karabo is a "hack" to avoid using extern template declarations
#       for the specializations for Hash and Schema of both the Binary and Text serializers. When extern template declarations are
#       used, the specializations won't make into the resulting object files and a linker error with undefined references to the
#       specializations will happen.
target_compile_definitions(dataIoTestRunner PRIVATE __SO__)
target_link_libraries(dataIoTestRunner spdlog::spdlog pugixml::pugixml cppunit::cppunit)
target_include_directories(dataIoTestRunner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
add_test(NAME dataIoTestRunner COMMAND dataIoTestRunner testresults/dataIoTestRunner.xml)

file(GLOB logTestRunner_FILES CMAKE_CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/log/*.cc")
list(APPEND logTestRunner_FILES $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>)

add_executable(logTestRunner ${karaboLog_FILES} ${dataTypes_FILES} ${dataSchema_FILES} ${dataTime_FILES} ${logTestRunner_FILES})
target_compile_options(logTestRunner PRIVATE -DKARABO_TESTPATH=\"${CMAKE_CURRENT_SOURCE_DIR}/\")
target_link_libraries(logTestRunner spdlog::spdlog cppunit::cppunit)
target_include_directories(logTestRunner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
add_test(NAME logTestRunner COMMAND logTestRunner testresults/logTestRunner.xml)


# Unit tests that link to libkarabo.so
set(TEST_TARGETS
    coreTestRunner
    dataSchemaTestRunner
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
    cppunit::cppunit
)

add_library(TEST_RUNNER OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/testRunner.cc)
target_link_libraries(
    TEST_RUNNER
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

file(GLOB dataSchemaTestRunner_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/data/schema/*.cc")
list(APPEND dataSchemaTestRunner_FILES $<TARGET_OBJECTS:TEST_RUNNER_NOLOOP>)

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
