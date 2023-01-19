# Project that builds the C++ Karabo Framework Integration Tests.
#
# Variables that influence the configuration behavior and that should be
# explicitly set at project configuration and generation time:

# CMAKE_PREFIX_PATH: when the target named "karabo" (the Karabo Shared Library 
#                    target) is not known, this variable value should have the
#                    root directory of the external build (and run) time 
#                    dependencies of the karabo shared library.
#

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-cpp-integration-tests"
    LANGUAGES C CXX
)

include("../cmake/cxx-options.cmake")
include("../cmake/karabo-lib-target-name.cmake")
include("../cmake/resolve-karabo-lib-target.cmake")

enable_testing()

find_library(CPPUNIT cppunit)

resolveKaraboLibTarget()

# At this point either the Karabo Lib target was defined in a
# parent project or it has been found by find_package - the config
# package defines the Karabo Lib target as an imported one and
# assigns it the name "karabo".
# It is safe to go ahead and use the Karabo Lib target as a
# dependency of the test runners.

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testresults)

add_library(INTEGRATION_RUNNER OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner.cc)
target_link_libraries(
    INTEGRATION_RUNNER
    ${KARABO_LIB_TARGET_NAME}
    ${CPPUNIT}
)
add_library(TCP_ADAPTER OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/TcpAdapter.cc)
target_link_libraries(
    TCP_ADAPTER
    ${KARABO_LIB_TARGET_NAME}
    ${CPPUNIT}
)


set(INTEGRATION_TEST_TARGETS
    alarmServiceIntegrTestRunner
    dataLoggingIntegrTestRunner
    deviceIntegrTestRunner
    deviceLockingIntegrTestRunner
    guiServerIntegrTestRunner
    pipelinedProcessingIntegrTestRunner
    propertyTestIntegrTestRunner
    runtimeSchemaAttrsIntegrTestRunner
    sceneProviderIntegrTestRunner
    timingIntegrTestRunner
)

set(alarmServiceIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/AlarmService_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AlarmTesterDevice.cc
    $<TARGET_OBJECTS:TCP_ADAPTER>
    $<TARGET_OBJECTS:INTEGRATION_RUNNER>
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

set(deviceLockingIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LockTestDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LockTest_Test.cc
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

set(runtimeSchemaAttrsIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/AlarmTesterDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RunTimeSchemaAttributes_Test.cc
    $<TARGET_OBJECTS:TCP_ADAPTER>
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
        ${CPPUNIT}
    )
    message(STATUS "Added integration test executable ${testTarget}")

endforeach()
