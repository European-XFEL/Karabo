# Project that builds the C++ Karabo Framework Long Running Tests.
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
# cmake -DCMAKE_PREFIX_PATH=$CONDA_PREFIX {{Karabo Framework Repository local working dir}}/src/cppLongTests
#

cmake_minimum_required(VERSION 3.14)

project(
    "karabo-cpp-long-tests"
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
    devicesLongTestRunner
    xmsLongTestRunner
)

file(GLOB devicesLongTestRunner_FILES "devices/*.cc")
file(GLOB xmsLongTestRunner_FILES "xms/*.cc")

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
