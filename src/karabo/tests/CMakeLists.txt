# The project that builds the C++ Karabo Framework Unit Tests.
#
# Variables that influence the configuration behavior and that should be
# explicitly set at project configuration and generation time:
#
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
# cmake -DCMAKE_PREFIX_PATH=$CONDA_PREFIX {{Karabo Framework Repository local working dir}}/src/karabo/tests
#

cmake_minimum_required(VERSION 3.12)

project(
    "karabo-cpp-unit-tests"
    LANGUAGES C CXX
)

include("../../../cmake/cxx-options.cmake")
include("../../../cmake/karabo-lib-target-name.cmake")
include("../../../cmake/resolve-karabo-lib-target.cmake")

enable_testing()

find_library(CPPUNIT cppunit)

resolveKaraboLibTarget()

# At this point either the Karabo Lib target was defined in a
# parent project or it has been defined as an imported target.
# It is safe to go ahead and use the Karabo Lib target as a
# dependency of the test runners.

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testresults)

# Some unit tests, more specifically io/H5File_test.cc, include header files
# using paths relative to the root level of the source tree. To support those
# without modifying any test source file, that directory is added in here.
set(KARABO_ROOT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Unit tests - under current directory, which is the root of the
# unit tests.
file(GLOB TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/*")

foreach(TEST_DIR ${TEST_DIR})
    if (NOT IS_DIRECTORY ${TEST_DIR})
        continue()  # We're only interested in directories.
    endif()
    file(GLOB TEST_SRCS "${TEST_DIR}/*.cc")
    foreach (TMP_PATH ${TEST_SRCS})
        string (FIND ${TMP_PATH} "TestRunner" NAME_FOUND)
        if (NOT ${NAME_FOUND} EQUAL -1)
            get_filename_component(RUNNER_NAME ${TMP_PATH} NAME_WE)
            break()
        endif ()
    endforeach()
    add_executable(${RUNNER_NAME} ${TEST_SRCS})
    target_include_directories(${RUNNER_NAME} PRIVATE ${KARABO_ROOT_SRC_DIR})
    add_test(NAME ${RUNNER_NAME} COMMAND ${RUNNER_NAME})
    message(STATUS "Added unit test executable ${RUNNER_NAME}")
    target_compile_options(
        "${RUNNER_NAME}"
        PRIVATE -DKARABO_TESTPATH=\"${CMAKE_CURRENT_BINARY_DIR}/\"
    )

    target_link_libraries(
        ${RUNNER_NAME}
        ${KARABO_LIB_TARGET_NAME}
        ${CPPUNIT}
    )

endforeach()

# The io unit tests depend on some data files to succeed - copy them
# to the build directory.
add_custom_command(
    TARGET ioTestRunner
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/io/resources"
        "${CMAKE_CURRENT_BINARY_DIR}/io/resources"
)
