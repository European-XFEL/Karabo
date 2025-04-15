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
# The project that builds the various C++ components of the Karabo Framework:
# libkarabo, C++ device server, unit tests, integration tests,
# and karabo C++ utilities like BrokerMessageLogs, BrokerMessageRates, ...
#
# Variables that influence the configuration behavior and that should be
# explicitly set at project configuration and generation time:
#
# BUILD_UNIT_TESTING: if false (default is true), skips generation of
#                     unit tests.
#
# BUILD_INTEGRATION_TESTING: if true (default is false), integration tests are
#                            generated.
#
# BUILD_LONG_RUN_TESTING: if true (default is false), long running integration
#                         tests are generated.
#
# GENERATE_CODE_COVERAGE: if true (default is false), generates a test  code
#                         coverage report for all the tests to be built.
#
# CMAKE_PREFIX_PATH: this is used by the projects to find their external
#                    dependencies - e.g. libkarabo depends on Boost and the
#                    tests depend on CppUnit. The variable should point to the
#                    directory that host the header files and libs for the
#                    dependencies. If you are using Visual Studio Code there is
#                    a Python script, "$REPO_ROOT/build/karabo/setupVSCodeCMake.py"
#                    that generates a settings file containing among other things
#                    the right CMAKE_PREFIX_PATH for your system.
#
# Example of command line to configure and generate the build:
#
# cmake -DCMAKE_PREFIX_PATH=/home/user/Framework/extern/Ubuntu-20-x86_64
#       -DBUILD_UNIT_TESTING=1 -DBUILD_INTEGRATION_TESTING=1
#       -DBUILD_LONG_RUN_TESTING=1 ../Framework
#

cmake_minimum_required(VERSION 3.15)

project(
    "karabo-cpp"
    LANGUAGES C CXX
)

message(
    STATUS
    "Using cmake ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION} (${CMAKE_GENERATOR})"
)

include("cmake/cxx-options.cmake")
include("cmake/karabo-lib-target-name.cmake")

# Sets the default values for the build configuration variables.
# The settings below are overridden by any previously set value (e.g. defined by
# the user in the command line).
set(BUILD_UNIT_TESTING ON CACHE BOOL "Should build Framework unit tests?")
set(BUILD_INTEGRATION_TESTING OFF CACHE BOOL "Should build Framework integration tests?")
set(BUILD_LONG_RUNNING_TESTING OFF CACHE BOOL "Should build Framework long running tests?")
set(GEN_CODE_COVERAGE OFF CACHE BOOL "Generate test code coverage report?")

if (GEN_CODE_COVERAGE)
    if(NOT DEFINED ENV{LCOV_PATH_HINT})
        set(ENV{LCOV_PATH_HINT} ${CMAKE_CURRENT_SOURCE_DIR}/../ci/coverage/lcov)
    endif()
    include("cmake/code-coverage.cmake")
    append_coverage_compiler_flags()
endif()

# Defines supported build types and sets the default build type to be "Debug"
# (the default build type can vary between different platforms/toolchains).
get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT isMultiConfig)
    set(allowableBuildTypes Debug Release)
    set_property(
        CACHE
        CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowableBuildTypes}"
    )
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowableBuildTypes)
        message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
    endif()
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release" AND GEN_CODE_COVERAGE)
    message(FATAL_ERROR
        "Code coverage reports cannot be produced by Release builds."
    )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Creates file "compile_commands.json"
                                         # in the build directory. It is useful
                                         # for integrating with external tools
                                         # such as "clangd".
endif()

# The build of the C++ Karabo Framework Library (libkarabo.so).
# For now, the binary output dir is set to have the same name of
# the project contained in the subdirectory. It cannot use the
# current CMAKE_BINARY_DIR directly because there could be some
# overwritten files, e.g., "Makefile" if the makefiles generator
# is used.
add_subdirectory(karabo ${CMAKE_BINARY_DIR}/karabo)

# The build of the Karabind Library (used by the Bound Python API).
add_subdirectory(karabind ${CMAKE_BINARY_DIR}/karabind)

# The build of the Karabo C++ Device Server (karabo-cppserver).
add_subdirectory(deviceServer ${CMAKE_BINARY_DIR}/karabo/deviceServer)

# The build of the Karabo Broker Message Logger utility (karabo-brokermessagelogger)
add_subdirectory(brokerMessageLogger ${CMAKE_BINARY_DIR}/karabo/brokerMessageLogger)

# The build of the Karabo Broker Rates utility (karabo-brokerrates)
add_subdirectory(tools/brokerRates ${CMAKE_BINARY_DIR}/karabo/brokerRates)

# The build of the Karabo Idx Utilities for file based data logging (karabo-idxbuild and karabo-idxview)
add_subdirectory(tools/dataLoggerIndex ${CMAKE_BINARY_DIR}/karabo/dataLoggerIndex)

# Generates the script to activate an environment for running resulting Karabo artifacts
# in the project build tree.
set(KARABO_PATH ${CMAKE_BINARY_DIR}/karabo)
set(KARABO_EXECUTABLES_ARTIFACTS_PATHS
"\
${KARABO_PATH}/brokerMessageLogger:\
${KARABO_PATH}/brokerRates:\
${KARABO_PATH}/dataLoggerIndex:\
${KARABO_PATH}/deviceServer:\
${KARABO_PATH}/tests:\
${KARABO_PATH}/integrationTests\
${KARABO_PATH}/cppLongTests\
")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/karabo/cmake/activateKarabo.in"
    "${CMAKE_BINARY_DIR}/activateKarabo.sh"
    @ONLY
)

if (BUILD_UNIT_TESTING OR BUILD_INTEGRATION_TESTING OR BUILD_LONG_RUN_TESTING)

    if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        enable_testing()
    endif()

    if (GEN_CODE_COVERAGE)
        set(TEST_COVERAGE_TARGET "test_coverage_report")
        setup_target_for_coverage_lcov(
            NAME ${TEST_COVERAGE_TARGET}
            EXECUTABLE ctest -VV
            BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            LCOV_ARGS --no-external --quiet
            EXCLUDE "/usr/*" "*/boost/*" "*/mqtt/*" "*/cppunit/*" "*/nlohmann/*"
                    "*/tests/*" "*/include/python*"
                    "*/include/numpy/*" "${CMAKE_CURRENT_SOURCE_DIR}/tools/*"
                    "${CMAKE_CURRENT_SOURCE_DIR}/deviceServer/*"
                    "${CMAKE_CURRENT_SOURCE_DIR}/brokerMessageLogger/*"
                    "${CMAKE_CURRENT_SOURCE_DIR}/karabind/*"
		    "${CMAKE_CURRENT_SOURCE_DIR}/integrationTests/*"
                    "${CMAKE_CURRENT_SOURCE_DIR}/cppLongTests/*"
                    "*/karabo-extern-deps/*"
        )
        string(TIMESTAMP ZIP_FILE_TIMESTAMP "%Y-%m-%dT%H-%M-%S")
        set(REPORT_ZIP_FILE
            "${CMAKE_BINARY_DIR}/test_coverage_report_${ZIP_FILE_TIMESTAMP}.zip"
        )
        add_custom_command(TARGET ${TEST_COVERAGE_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E tar "cf" "${REPORT_ZIP_FILE}" --format=zip "${CMAKE_BINARY_DIR}/${TEST_COVERAGE_TARGET}"
            COMMENT "###\nZipped version of coverage report available at '${REPORT_ZIP_FILE}'\n###"
        )
    endif()
endif()

if (BUILD_UNIT_TESTING)
    add_subdirectory(karabo/tests ${CMAKE_BINARY_DIR}/karabo/tests)
endif()

if (BUILD_INTEGRATION_TESTING)
    add_subdirectory(
        integrationTests
        ${CMAKE_BINARY_DIR}/karabo/integrationTests
    )
endif()

if (BUILD_LONG_RUN_TESTING)
    add_subdirectory(
        cppLongTests
        ${CMAKE_BINARY_DIR}/karabo/cppLongTests
    )
endif()
