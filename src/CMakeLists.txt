# The project that builds the various C++ components of the Karabo
# Framework: karabo library itself, device server, unit tests,
# integration tests and karabo C++ utilities like BrokerMessageLogs,
# BrokerMessageRates, IdxBuild ...
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
#                    directory(ies) that host the includes and libs for the
#                    required dependencies (most likely it will be the path to
#                    a Conda environment with all the dependencies installed as
#                    packages).
#
# Example of command line to configure and generate the build:
#
# cmake -DCMAKE_PREFIX_PATH=/home/user/miniconda3/envs/karabo-cpp
#       -DBUILD_UNIT_TESTING=1 -DBUILD_INTEGRATION_TESTING=1
#       -DBUILD_LONG_RUN_TESTING=1 ../Framework
#

# TODO: Add a BUILD_KARATHON variable that when true (the default) will build
#       the Karathon library.

# TODO: Add logic to update the Conda environment with the requirements for the
#       Karathon library to the CMakeLists.txt at the src/karathon directory
#       (the Karathon requirements Conda environment will be created).

cmake_minimum_required(VERSION 3.14)

project(
    "karabo-cpp"
    LANGUAGES C CXX
)

message(
    STATUS
    "Using cmake ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}"
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
        set(ENV{LCOV_PATH_HINT} ${CMAKE_SOURCE_DIR}/ci/coverage/lcov)
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
# current CMAKE_BINARY_DIR directly because it will also output its own
# Makefile that would clash with the one from the root directory.
add_subdirectory(karabo ${CMAKE_BINARY_DIR}/karabo)

# The build of the Karathon Library (used by the Bound Python API).
add_subdirectory(karathon ${CMAKE_BINARY_DIR}/karathon)

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
    "${CMAKE_SOURCE_DIR}/karabo/activateKarabo.in"
    "${CMAKE_BINARY_DIR}/activateKarabo.sh"
    @ONLY
)

if (BUILD_UNIT_TESTING OR BUILD_INTEGRATION_TESTING OR BUILD_LONG_RUN_TESTING)
    enable_testing()
    if (GEN_CODE_COVERAGE)
        set(TEST_COVERAGE_TARGET "test_coverage_report")
        setup_target_for_coverage_lcov(
            NAME ${TEST_COVERAGE_TARGET}
            EXECUTABLE ctest -VV
            BASE_DIRECTORY ${CMAKE_SOURCE_DIR}/src
            LCOV_ARGS --no-external --quiet
            EXCLUDE "/usr/*" "*/boost/*" "*/mqtt/*" "*/cppunit/*" "*/nlohmann/*"
                    "*/krb_log4cpp/*" "*/tests/*" "*/include/python*"
                    "*/include/numpy/*" "${CMAKE_SOURCE_DIR}/tools/*"
                    "${CMAKE_SOURCE_DIR}/deviceServer/*"
                    "${CMAKE_SOURCE_DIR}/brokerMessageLogger/*"
                    "${CMAKE_SOURCE_DIR}/karathon/*"
                    "${CMAKE_SOURCE_DIR}/integrationTests/*"
                    "${CMAKE_SOURCE_DIR}/cppLongTests/*"
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