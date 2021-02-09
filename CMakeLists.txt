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
# CMAKE_PREFIX_PATH: this is used by the project that builds the karabo library.
#                    Please take a look at "./src/karabo/CMakeLists.txt" for
#                    further details.
#
# Example of command line to configure and generate the build:
#
# cmake -DCMAKE_PREFIX_PATH=/home/user/miniconda3/envs/karabo_cpp
#       -DBUILD_UNIT_TESTING=1 -DBUILD_INTEGRATION_TESTING=1 ../Framework
#

cmake_minimum_required(VERSION 3.12)

project(
    "karabo-cpp"
    LANGUAGES C CXX
)

include("cmake/cxx-options.cmake")
include("cmake/karabo-lib-target-name.cmake")

if (BUILD_UNIT_TESTING OR BUILD_INTEGRATION_TESTING)
    enable_testing()
endif()

# Defines supported build types and sets the default build type to be "Debug"
# (the default build type can vary between different platforms/toolchains).
get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT isMultiConfig)
    # TODO: investigate how to support CODE_COVERAGE builds.
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

if (CMAKE_BUILD_TYPE STREQUAL Debug)
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
add_subdirectory(src/karabo ${CMAKE_BINARY_DIR}/karabo)

# The build of the Karabo C++ Device Server (karabo-cppserver).
# add_subdirectory(src/deviceServer ${CMAKE_BINARY_DIR}/karabo)

if (BUILD_UNIT_TESTING)
    add_subdirectory(src/karabo/tests ${CMAKE_BINARY_DIR}/karabo/tests)
endif()

if (BUILD_INTEGRATION_TESTING)
    add_subdirectory(
        src/integrationTests
        ${CMAKE_BINARY_DIR}/karabo/integrationTests
    )
endif()
