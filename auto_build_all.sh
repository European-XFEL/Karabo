#!/bin/bash

# Script for automated building and packaging of the entire karaboFramework.
# Uses cmake 3.14+ for building the C++ components of the karaboFramework.
#
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
#

# Help function for checking successful execution of commands
safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code
    tmp_output=$(mktemp)
    echo cmnd=$cmnd
    if [ -z "$KARABO_CI_QUIET" ]; then
        exec 3>&1
    else
        exec 3>"$tmp_output"
    fi
    eval $cmnd>&3 2>&3
    ret_code=$?
    if [ $ret_code != 0 ]; then
        cat $tmp_output
        rm -f $tmp_output
        exec 3>&1
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        echo
        echo
        exit $ret_code
    fi
    rm -f $tmp_output
    exec 3>&1
}

activateKarabo() {
    if [ -z "$KARABO" ]; then
        source $scriptDir/karabo/activate
    fi
    # set the broker variable if KARABO_TEST_BROKER is set
    if [ -n "$KARABO_TEST_BROKER" ]; then
        export KARABO_BROKER=$KARABO_TEST_BROKER
    fi
}

activateKaraboBuildTree() {
    # Partially activates Karabo in cmake's build tree.
    # Details on what is activated can be seen on the "activateKarabo.sh" script.
    source $FRAMEWORK_BUILD_DIR/activateKarabo.sh
    # set the broker variable if KARABO_TEST_BROKER is set
    if [ -n "$KARABO_TEST_BROKER" ]; then
        export KARABO_BROKER=$KARABO_TEST_BROKER
    fi
}

checkCppUnitTestResults() {
    # activate karabo to get a python environment
    activateKarabo
    local mergeArgs=""
    safeRunCommand "python -m pip install --upgrade ${scriptDir}/ci/utils/cppunitxmlparser/."
    for name in $(ls ${FRAMEWORK_BUILD_DIR}/karabo/*/testresults/*.xml); do
        mergeArgs="${mergeArgs} ${name}"
    done
    if [[ ${mergeArgs} = "" ]]; then
        echo "ERROR! No XML output file was generated!"
        exit 1
    fi
    # process the cppunit report files into a junit compatible file
    cppunitxml-check -f${mergeArgs} -o $scriptDir/junit.cpp.xml
    # this should have a non-zero return value if errors are present.
    ret_code=$?
    if [ $ret_code != 0 ]; then
        exit $ret_code
    fi
    deactivate
}


runPythonUnitTests() {
    activateKarabo

    echo
    echo Running Karabo Python unit tests ...
    echo

    if [ $CODECOVERAGE = "y" ]; then
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runUnitTests \
            --collectCoverage \
            --rootDir $scriptDir
    else
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runUnitTests \
            --rootDir $scriptDir
    fi
    deactivate
}

runPythonIntegrationTests() {
    activateKarabo

    echo
    echo Running Karabo Python integration tests ...
    echo

    if [ $CODECOVERAGE = "y" ]; then
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runIntegrationTests \
            --collectCoverage \
            --rootDir $scriptDir
    else
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runIntegrationTests \
            --rootDir $scriptDir
    fi
    deactivate
}

runPythonLongTests() {
    activateKarabo

    echo
    echo Running Karabo Python long running tests ...
    echo

    if [ $CODECOVERAGE = "y" ]; then
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runLongTests \
            --collectCoverage \
            --rootDir $scriptDir
    else
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runLongTests \
            --rootDir $scriptDir
    fi
    deactivate
}

producePythonCodeCoverageReport() {
    echo "### Producing code coverage reports for Python tests ..."
    echo

    # Needed for 'run_python_tests.sh --clean ...':
    activateKarabo

    # remove any previous code coverage results
    safeRunCommand "find . -name \"*.gcda\" -delete"
    safeRunCommand $scriptDir/run_python_tests.sh --clean --rootDir $scriptDir

    runPythonUnitTests
    runPythonIntegrationTests
    # To include long running Python tests in the coverage report, uncomment
    # the following line - they currently don't influence the coverage metrics
    # significantly and add a long time to the CI job execution.
    # runPythonLongTests

    # produce initial Python coverage information
    safeRunCommand $scriptDir/run_python_tests.sh \
        --generateCoverageReport \
        --rootDir $scriptDir \
        --reportDir $scriptDir/ci/coverage

    echo
    deactivate
}

# Make sure the script runs in the correct directory
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
cd ${scriptDir}
if [ $? -ne 0 ]; then
    echo " Could not change directory to ${scriptDir}"
    exit 1;
fi

# Parse command line
if [[ -z "$1" ||  $1 = "help" || $1 = "-h" ||  $1 = "-help" || $1 = "--help" ]]; then
    cat <<End-of-help
Usage: $0 Debug|Release|CodeCoverage|Clean|Clean-All [flags]

Available flags:
  --bundle     - Installs Karabo and creates the software bundle. Default: no bundle is created!
  --pyDevelop  - Install Python packages in development mode
  --buildAllTests
               - Build all test executables, but do not execute them
  --skipCppTests
               - Skips C++ tests
  --skipPythonTests
               - Skips Python tests
  --runTests   - Run unit tests after building (useful for Debug|Release)
  --runIntegrationTests
               - Run integration tests after building (for Debug|Release)
  --runLongTests
               - Run long running tests after building (for Debug|Release)
  --numJobs N  - Specify the number of build jobs to run simultaneously. If omitted,
                 the value of the NUM_COMPILE_JOBS environment variable is used. If
                 the variable is not defined, the number of active machine cores is
                 queried and used.
  --quiet      - suppress commands' stdout on success

Note: "Clean" cleans all Karabo code (src folder)
      "Clean-All" additionally cleans all external dependencies.
      "CodeCoverage" builds the Karabo framework with CodeCoverage configuration,
                     but also implicitly runs the unit and integration tests
                     and produces code coverage reports. The CodeCoverage configuration
                     also disables --pyDevelop option.
      The environment variable "KARABO_TEST_BROKER" can be set to force the unit
      tests to run on the broker set in this variable.
      Please note that tests will activate the Karabo installation currently
      built.
End-of-help

    exit 0
fi

# Get some information about our system
TARGET_ARCH=$(uname -m)
OS=$(uname -s)
source "$scriptDir/set_lsb_release_info.sh"
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=$LSB_RELEASE_DIST
    DISTRO_RELEASE=$(echo $LSB_RELEASE_VERSION | sed -r "s/^([0-9]+).*/\1/")
else
    echo "Karabo Framework is currently supported only on Linux"
    exit 1
fi

# External dependencies have to be outside the source tree. This is
# required by CMake since it doesn't allow a path in CMAKE_PREFIX_PATH to
# be internal to the source tree.
EXTERN_DEPS_BASE_DIR="$scriptDir/extern"
EXTERN_DEPS_DIR="$EXTERN_DEPS_BASE_DIR/$DISTRO_ID-$DISTRO_RELEASE-$TARGET_ARCH"

# Fetch configuration type (Release or Debug)
if [[ $1 = "Release" || $1 = "Debug" || $1 = "CodeCoverage" ]]; then
    CONF=$1
    if [[ ( $CONF = "Debug" || $CONF = "Release" ) ]]; then
        CMAKE_CONF=$CONF
    elif [ $CONF = "CodeCoverage" ]; then
        # CodeCoverage requires Debug cmake build configurations
        CMAKE_CONF="Debug"
    fi
elif [[ $1 = "Clean" || $1 = "Clean-All" ]]; then
    echo "### Cleaning build output directories for all configurations ###"
    if [ -d $scriptDir/build_debug ]; then
        rm -rf $scriptDir/build_debug
        echo "    removed $scriptDir/build_debug"
    fi
    if [ -d $scriptDir/build_release ]; then
        rm -rf $scriptDir/build_release
        echo "    removed $scriptDir/build_release"
    fi
    if [ -d $scriptDir/build_codecoverage ]; then
        rm -rf $scriptDir/build_codecoverage
        echo "    removed $scriptDir/build_codecoverage"
    fi
    if [[ $1 = "Clean-All" ]]; then
        echo "### Cleaning extern (third-party dependencies) directory ###"
        # Also cleans the extern artifacts
        if [ -d $EXTERN_DEPS_DIR ]; then
            rm -rf $EXTERN_DEPS_DIR
            echo "    removed $EXTERN_DEPS_DIR"
        fi
    fi
    exit 0
else
    echo
    echo "Invalid option supplied. Allowed options: Release|Debug|CodeCoverage|Clean|Clean-All"
    echo
    exit 1
fi

# Get rid of the first argument
shift

# Parse the commandline flags
BUNDLE="n"
BUILDALLTESTS="n"
RUNTESTS="n"
RUNINTEGRATIONTESTS="n"
RUNLONGTESTS="n"
SKIP_PYTHON_TESTS="n"
SKIP_CPP_TESTS="n"
PYOPT="normal"
NUM_JOBS=0
CODECOVERAGE="n"
while [ -n "$1" ]; do
    case "$1" in
        --bundle)
            # Don't skip bundling
            BUNDLE="y"
            ;;
        --pyDevelop)
            # Build Python packages in development mode
            PYOPT="develop"
            ;;
        --buildAllTests)
            # Build all tests
            BUILDALLTESTS="y"
            ;;
        --runTests)
            # Run all the unit tests too
            RUNTESTS="y"
            ;;
        --runIntegrationTests)
            # Run the integration tests
            RUNINTEGRATIONTESTS="y"
            ;;
        --runLongTests)
            RUNLONGTESTS="y"
            ;;
        --skipPythonTests)
            SKIP_PYTHON_TESTS="y"
            ;;
        --skipCppTests)
            SKIP_CPP_TESTS="y"
            ;;
        --numJobs)
            # Limit the numbers of jobs for make runs
            if [ -n "$2" ]; then
                NUM_JOBS=$2
                shift
            else
                echo "Option --numJobs needs a number of jobs"
                exit 1
            fi
            ;;
        --quiet)
            KARABO_CI_QUIET="y"
            ;;
        *)
            # Make a little noise
            echo "Unrecognized commandline flag: $1"
            ;;
    esac
    shift
done

if [[ $BASH_VERSINFO -lt 4 ]]; then
    echo "Bash version 4 or higher required"
    exit 1
fi

if [ "$NUM_JOBS" = "0" ]; then
    # numJobs not specified in command-line;
    if [ -z "$NUM_COMPILE_JOBS" ]; then
       # numJobs not specified in NUM_COMPILE_JOBS environment variable; use
       # the number of active cores as the final fallback strategy.
       NUM_JOBS=`grep "processor" /proc/cpuinfo | wc -l`
    else
       NUM_JOBS=$NUM_COMPILE_JOBS
    fi
fi

# selecting configuration CodeCoverage implies --runTests, --runIntegrationTests and
# --runLongTests called by the code coverage function. Also, other options are disabled.
# No need to run those separately, so we turn them off explicitly in case the user specified them.
if [ "$CONF" = "CodeCoverage" ]; then
    CODECOVERAGE="y"
    RUNTESTS="n"
    RUNINTEGRATIONTESTS="n"
    RUNLONGTESTS="n"
    PYOPT="normal"
fi

sleep 2

# Sets the test building and execution variables
if [ "$RUNTESTS" = "y" ] && [ "$SKIP_CPP_TESTS" = "n" ]; then
    BUILD_UNIT_TESTING="1"
else
    BUILD_UNIT_TESTING="0"
fi
if [ "$RUNINTEGRATIONTESTS" = "y" ] && [ "$SKIP_CPP_TESTS" = "n" ]; then
    BUILD_INTEGRATION_TESTING="1"
else
    BUILD_INTEGRATION_TESTING="0"
fi
if [ "$RUNLONGTESTS" = "y" ] && [ "$SKIP_CPP_TESTS" = "n" ]; then
    BUILD_LONG_RUN_TESTING="1"
else
    BUILD_LONG_RUN_TESTING="0"
fi

if [ "$BUILDALLTESTS" = "y" ]; then
    BUILD_UNIT_TESTING="1"
    BUILD_INTEGRATION_TESTING="1"
    BUILD_LONG_RUN_TESTING="1"
fi

if [ "$CODECOVERAGE" = "y" ]; then
    GEN_CODE_COVERAGE="1"
    # For CodeCoverage, the long running tests are disabled due to the Telegraf
    # based tests, that are currently unreliable. Setting BUILD_LONG_RUN_TESTING
    # to 1 below will include the long running tests in the coverage report.
    BUILD_UNIT_TESTING="1"
    BUILD_INTEGRATION_TESTING="1"
    BUILD_LONG_RUN_TESTING="0"
    RUNINTEGRATIONTESTS="y"
    RUNTESTS="y"
    RUNLONGTESTS="n"
else
    GEN_CODE_COVERAGE="0"
fi

echo "### Now building and packaging Karabo... ###";

mkdir -p $EXTERN_DEPS_BASE_DIR
# Uses the specified number of jobs for building any dependency whose build is
# driven by CMake.
export CMAKE_BUILD_PARALLEL_LEVEL=$NUM_JOBS
safeRunCommand $scriptDir/extern/build.sh $EXTERN_DEPS_DIR ALL

# Use Ninja as the CMAKE Generator if it is available and if no setting for
# using a given Generator already exists.
which ninja &> /dev/null
if [[ $? = 0 && -z "$CMAKE_GENERATOR" ]]; then
    export CMAKE_GENERATOR="Ninja"
    echo
    echo "### Using ninja as the CMAKE_GENERATOR... ###"
    echo
elif [ -n "$CMAKE_GENERATOR" ]; then
    echo
    echo "### Using $CMAKE_GENERATOR as the CMAKE_GENERATOR... ###"
    echo
else
    echo
    echo "### Using the default CMAKE_GENERATOR ('cmake -h' shows the default)... ###"
    echo
fi

LOWER_CMAKE_CONF=$(echo "$CONF" | tr '[:upper:]' '[:lower:]')
FRAMEWORK_BUILD_DIR="$scriptDir/build_$LOWER_CMAKE_CONF"
mkdir -p $FRAMEWORK_BUILD_DIR
FRAMEWORK_INSTALL_DIR="$scriptDir/package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$TARGET_ARCH/karabo"

pushd $FRAMEWORK_BUILD_DIR

# Use the cmake from the EXTERN_DEPS_DIR - cmake is an external dependency
# of Karabo as it is used to compile some other dependencies, like the
# MQTT client lib.
# NOTE:
# Please keep in sync with the cmake.configureSettings generated in
# build/karabo/setupVSCodeCMake.py.
safeRunCommand $EXTERN_DEPS_DIR/bin/cmake -DCMAKE_PREFIX_PATH=$EXTERN_DEPS_DIR \
    -DBUILD_UNIT_TESTING=$BUILD_UNIT_TESTING \
    -DBUILD_INTEGRATION_TESTING=$BUILD_INTEGRATION_TESTING \
    -DBUILD_LONG_RUN_TESTING=$BUILD_LONG_RUN_TESTING \
    -DGEN_CODE_COVERAGE=$GEN_CODE_COVERAGE \
    -DCMAKE_MAP_IMPORTED_CONFIG_DEBUG=Release \
    -DCMAKE_TOOLCHAIN_FILE=$EXTERN_DEPS_DIR/conan_toolchain/conan_toolchain.cmake \
    -DCMAKE_INSTALL_PREFIX=$FRAMEWORK_INSTALL_DIR \
    -DCMAKE_BUILD_TYPE=$CMAKE_CONF \
    $scriptDir/src/.

if [ $? -ne 0 ]; then
    echo
    echo "#### Error on cmake project configuration phase. Exiting. ####"
    exit 1
fi

# Cleans-up the installation dir, skipping some specific directories that
# should be preserved.
if [ -d $FRAMEWORK_INSTALL_DIR ]; then
    for dir in $FRAMEWORK_INSTALL_DIR/*
    do
        if [[ "$dir" = "$FRAMEWORK_INSTALL_DIR/devices" ||
              "$dir" = "$FRAMEWORK_INSTALL_DIR/installed" ||
              "$dir" = "$FRAMEWORK_INSTALL_DIR/plugins" ||
              "$dir" = "$FRAMEWORK_INSTALL_DIR/var" ]]; then
            : # Skip removal of directory
        else
            rm -rf $dir
        fi
    done
fi

# Builds libkarabo, libkarabind, karabo-* utilities
# and installs them in FRAMEWORK_INSTALL_DIR.
safeRunCommand $EXTERN_DEPS_DIR/bin/cmake --build . -j $NUM_JOBS --target install
if [ $? -ne 0 ]; then
    echo
    echo "#### Error on cmake project building phase. Exiting. ####"
    exit 1
fi

# Installs the components of the Karabo Framework that are not built
# with CMake into FRAMEWORK_INSTALL_DIR - the Python tests must be
# run from the activated Karabo environment hosted in the install
# tree.
BUNDLE_ACTION="install"
if [ "$BUNDLE" = "y" ]; then
    BUNDLE_ACTION="package"
fi
safeRunCommand $scriptDir/build/karabo/bundle.sh dist $CONF $PLATFORM $BUNDLE_ACTION $PYOPT $EXTERN_DEPS_DIR

# enable prints from now on.
if [ ! -z "$KARABO_CI_QUIET" ]; then
    unset KARABO_CI_QUIET
fi

echo "### Successfully finished building of karaboFramework ###"

if [ "$GEN_CODE_COVERAGE" = "1" ]; then
    echo
    echo Running Karabo C++ tests and generating coverage report ...
    echo
    # Activate the karabo environment to allow tests to be run from the
    # build tree.
    activateKaraboBuildTree
    # When GEN_CODE_COVERAGE is true, the cmake configuration phase generates
    # a 'test_coverage_report' target, than when built runs the tests and
    # generates the coverage report.
    $EXTERN_DEPS_DIR/bin/cmake --build . -j $NUM_JOBS --target test_coverage_report
    deactivateKarabo
    # Generate the coverage report for the Python tests - all of them.
    producePythonCodeCoverageReport
    exit 0
fi

if [ "$SKIP_CPP_TESTS" = "n" ] && { [ "$RUNTESTS" = "y" ] || [ "$RUNINTEGRATIONTESTS" = "y" ] || [ "$RUNLONGTESTS" = "y" ];}; then
    echo
    echo Running Karabo C++ tests ...
    echo
    # Activate the karabo environment to allow tests to be run from the
    # build tree.
    typeset tests_ret_code
    activateKaraboBuildTree
    # clear previous tests
    for name in $(ls ${FRAMEWORK_BUILD_DIR}/karabo/*/testresults/*.xml); do
        rm -f ${name}
    done
    # NOTE: the tests are not executed inside a `safeRunCommand` function.
    #       this is to give us the chance to process the result files.
    ctest -VV
    tests_ret_code=$?
    # deactivate the environment where C++ tests are run
    deactivateKarabo
    # Parse the XML test outputs
    checkCppUnitTestResults
    if [ $tests_ret_code != 0 ]; then
        echo "Test execution FAILED"
        echo "Report files processing did not find errors..."
        echo "A test file is likely missing due to a segmentation fault in tests."
        exit $tests_ret_code
    fi
fi

popd

if [ "$RUNTESTS" = "y" ] && [ "$SKIP_PYTHON_TESTS" = "n" ] ; then
    runPythonUnitTests
fi

if [ "$RUNINTEGRATIONTESTS" = "y" ] && [ "$SKIP_PYTHON_TESTS" = "n" ]; then
    runPythonIntegrationTests
fi

if [ "$RUNLONGTESTS" = "y" ] && [ "$SKIP_PYTHON_TESTS" = "n" ]; then
    runPythonLongTests
fi

echo "### Successfully finished building and packaging of karaboFramework ###"
echo
exit 0
