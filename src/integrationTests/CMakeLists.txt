# Project that builds the C++ Karabo Framework Integration Tests.
#
# Variables that influence the configuration behavior and that should be
# explicitly set at project configuration and generation time:

# CMAKE_PREFIX_PATH: when the target named "karabo" (the Karabo Shared Library target)
#                    is not known, this variable value should point to the root
#                    directory of an activated Conda environment. It can also have
#                    default value as initialized by CMAKE for a Docker Image or for a
#                    Physical System (for Ubuntus this is "/usr/local"). In all three
#                    cases, namely Conda, Docker Image or Physical System, the build
#                    dependencies of the Karabo Framework lib as well as the Karabo
#                    Framework shared lib itself must be available for this script
#                    to succeed.
#
# Example of command line to configure and generate the build for this project inside
# an activated Conda environment with the Karabo Framework lib package (and its dependencies)
# installed:
#
# cmake -DCMAKE_PREFIX_PATH=$CONDA_PREFIX {{Karabo Framework Repository local working dir}}/src/integrationTests
#

cmake_minimum_required(VERSION 3.12)

project(
    "karabo-cpp-integration-tests"
    LANGUAGES C CXX
)

include("../../cmake/cxx-options.cmake")
include("../../cmake/karabo-lib-target-name.cmake")
include("../../cmake/resolve-karabo-lib-target.cmake")

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
    ${CMAKE_CURRENT_SOURCE_DIR}/TcpAdapter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner.cc
)

set(dataLoggingIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/BaseLogging_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/DataLogging_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_11.cc
)

set(deviceIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Device_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_9.cc
)

set(deviceLockingIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LockTestDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LockTest_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_5.cc
)

set(guiServerIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/GuiServer_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TcpAdapter_5.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_10.cc
)

set(pipelinedProcessingIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/P2PSenderDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PipeReceiverDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PipelinedProcessing_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_4.cc
)

set(propertyTestIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/PropertyTest_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_2.cc
)

set(runtimeSchemaAttrsIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/AlarmTesterDevice_3.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RunTimeSchemaAttributes_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TcpAdapter_3.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_3.cc
)

set(sceneProviderIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/NonSceneProviderTestDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneProviderTestDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneProvider_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TcpAdapter_4.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_7.cc
)

set(timingIntegrTestRunner_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/SimulatedTimeServerDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TimingTestDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Timing_Test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/integrationRunner_8.cc
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
        "karabo"
        ${CPPUNIT}
    )
    message(STATUS "Added integration test executable ${testTarget}")

endforeach()
