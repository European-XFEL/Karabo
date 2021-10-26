#!/bin/bash

# Script for automated building and packaging of the entire karaboFramework.
# Uses cmake 3.14+ for building the C++ components of the karaboFramework.
#
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
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

checkCppUnitTestResults() {
    local testDir="$1"
    local testNames="$2"
    local mergeArgs=""
    safeRunCommand "python -m pip install --upgrade $scriptDir/ci/utils/cppunitxmlparser/."
    for name in $testNames; do
        local path=$(printf "%s/testresults/%sTest.xml" $testDir $name)
        mergeArgs="${mergeArgs} ${path}"
    done
    cppunitxml-check -f${mergeArgs} -o cpp.junit.xml
    ret_code=$?
    if [ $ret_code != 0 ]; then
        # the output file is expected to be 'junitoutput.xml'
        # by the CI application, if we do not fail here, the python
        # tests will aggreagate this file into a global result file.
        mv cpp.junit.xml junitoutput.xml
        exit $ret_code
    fi
    # Fail when new test suites are added and we don't expect them
    pushd $testDir/testresults/ &> /dev/null
    local actualNames=$(ls *.xml | cut -d'.' -f1 | tr '\n' ' ')
    popd &> /dev/null

    local expectedCount=$(echo $testNames | wc -w)
    local actualCount=$(echo $actualNames | wc -w)
    if [ "$expectedCount" != "$actualCount" ]; then
        echo "Test result files in '$testDir/testresults' don't match expectation:"
        echo "    Expected: $testNames"
        echo "    Got: $actualNames"
        echo
        echo
        exit 1
    fi
}


runPythonUnitTests() {
    activateKarabo

    local testNames=$(ls $scriptDir/src/karabo/tests --ignore=CMakeLists.txt)
    local testDir=$scriptDir/build/netbeans/karabo

    echo
    echo Running Karabo Python unit tests ...
    echo

    if [ $CODECOVERAGE = "y" ]; then
        # Collect code coverage.
        # Could force completion despite of test failures by not using
        # safeRunCommand and adding option --force.
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runUnitTests \
            --collectCoverage \
            --rootDir $scriptDir
    else
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runUnitTests \
            --rootDir $scriptDir
    fi
}

runPythonIntegrationTests() {
    activateKarabo

    echo
    echo Running Karabo Python integration tests ...
    echo

    if [ $CODECOVERAGE = "y" ]; then
        # Collect code coverage.
        # Could force completion despite of test failures by not using
        # safeRunCommand and adding option --force.
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runIntegrationTests \
            --collectCoverage \
            --rootDir $scriptDir
    else
        safeRunCommand $scriptDir/run_python_tests.sh \
            --runIntegrationTests \
            --rootDir $scriptDir
    fi
}

runPythonLongTests() {
    activateKarabo

    echo
    echo Running Karabo Python long running tests ...
    echo

    if [ $CODECOVERAGE = "y" ]; then
       # Collect code coverage.
       safeRunCommand $scriptDir/run_python_tests.sh \
           --runLongTests \
           --collectCoverage \
           --rootDir $scriptDir
    else
       safeRunCommand $scriptDir/run_python_tests.sh \
           --runLongTests \
           --rootDir $scriptDir
    fi
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
  --runTests   - Run unit tests after building (useful for Debug|Release)
  --runIntegrationTests
               - Run integration tests after building (for Debug|Release)
  --runLongTests
               - Run long running tests after building (for Debug|Release)
  --numJobs N  - Specify the number of build jobs to run simultaneously
  --quiet      - suppress commands' stdout on success

Note: "Clean" cleans all Karabo code (src folder)
      "Clean-All" additionally cleans all external dependencies (extern folder)
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
MACHINE=$(uname -m)
OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs | sed -r "s/^([0-9]+).*/\1/")
fi

# External dependencies have to be outside the source tree. This is
# required by CMake since it doesn't allow a path in CMAKE_PREFIX_PATH to
# be internal to the source tree.
EXTERN_DEPS_BASE_DIR="$scriptDir/extern"
EXTERN_DEPS_DIR="$EXTERN_DEPS_BASE_DIR/$DISTRO_ID-$DISTRO_RELEASE-$MACHINE"

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
        git clean -fxd $scriptDir/extern/resources
        echo "    Did git clean -fxd on $scriptDir/extern/resources"
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
RUNTESTS="n"
RUNINTEGRATIONTESTS="n"
RUNLONGTESTS="n"
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

if [ "$NUM_JOBS" = "0" ]; then
    # numJobs not specified in command-line; use the number of active cores.
    NUM_JOBS=`grep "processor" /proc/cpuinfo | wc -l`
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
if [ "$RUNTESTS" = "y" ]; then
    BUILD_UNIT_TESTING="1"
else
    BUILD_UNIT_TESTING="0"
fi
if [ "$RUNINTEGRATIONTESTS" = "y" ]; then
    BUILD_INTEGRATION_TESTING="1"
else
    BUILD_INTEGRATION_TESTING="0"
fi
if [ "$RUNLONGTESTS" = "y" ]; then
    BUILD_LONG_RUN_TESTING="1"
else
    BUILD_LONG_RUN_TESTING="0"
fi
if [ "$CODECOVERAGE" = "y" ]; then
    GEN_CODE_COVERAGE="1"
    # For CodeCoverage, the long running tests are disabled due to the Telegraf
    # based tests, that are currently unreliable. Setting BUILD_LONG_RUN_TESTING
    # to 1 below will include the long running tests in the coverage report.
    BUILD_UNIT_TESTING="1"
    BUILD_INTEGRATION_TESTING="1"
    BUILD_LONG_RUN_TESTING="0"
else
    GEN_CODE_COVERAGE="0"
fi

echo "### Now building and packaging Karabo... ###";

mkdir -p $EXTERN_DEPS_BASE_DIR
$scriptDir/extern/build.sh $EXTERN_DEPS_DIR ALL

LOWER_CMAKE_CONF=$(echo "$CONF" | tr '[:upper:]' '[:lower:]')
FRAMEWORK_BUILD_DIR="$scriptDir/build_$LOWER_CMAKE_CONF"
mkdir -p $FRAMEWORK_BUILD_DIR
FRAMEWORK_INSTALL_DIR="$scriptDir/package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/karabo"

cd $FRAMEWORK_BUILD_DIR

if [ -d $FRAMEWORK_INSTALL_DIR ]; then
    # Preserve any installed device, plugin and all contents under var dir.
    PRESERV_DIR=`mktemp -d -p $scriptDir`
    if [ -d $FRAMEWORK_INSTALL_DIR/devices ]; then
        safeRunCommand mv $FRAMEWORK_INSTALL_DIR/devices $PRESERV_DIR/devices
    fi
    if [ -d $FRAMEWORK_INSTALL_DIR/installed ]; then
        safeRunCommand mv $FRAMEWORK_INSTALL_DIR/installed $PRESERV_DIR/installed
    fi
    if [ -d $FRAMEWORK_INSTALL_DIR/plugins ]; then
        safeRunCommand mv $FRAMEWORK_INSTALL_DIR/plugins $PRESERV_DIR/plugins
    fi
    if [ -d $FRAMEWORK_INSTALL_DIR/var ]; then
        safeRunCommand mv $FRAMEWORK_INSTALL_DIR/var $PRESERV_DIR/var
    fi
fi

# Use the cmake from the EXTERN_DEPS_DIR - cmake is an external dependency
# of Karabo as it is used to compile some other dependencies, like the
# MQTT client lib.
safeRunCommand $EXTERN_DEPS_DIR/bin/cmake -DCMAKE_PREFIX_PATH=$EXTERN_DEPS_DIR \
      -DBUILD_UNIT_TESTING=$BUILD_UNIT_TESTING \
      -DBUILD_INTEGRATION_TESTING=$BUILD_INTEGRATION_TESTING \
      -DBUILD_LONG_RUN_TESTING=$BUILD_LONG_RUN_TESTING \
      -DGEN_CODE_COVERAGE=$GEN_CODE_COVERAGE \
      -DCMAKE_INSTALL_PREFIX=$FRAMEWORK_INSTALL_DIR \
      -DCMAKE_BUILD_TYPE=$CMAKE_CONF \
      $scriptDir/src/.

if [ $? -ne 0 ]; then
    # Have to restore any preserved directory that might have been moved before
    # exiting the script.
    if [[  -n $PRESERV_DIR && -d $PRESERV_DIR ]]; then
        mv $PRESERV_DIR/* $FRAMEWORK_INSTALL_DIR
        if [ $? -ne 0 ]; then
            echo "WARNING: Failed to restore content from \"$PRESERVE_DIR\" into \"$FRAMEWORK_INSTALL_DIR\"."
            echo "         Please try to restore it manually."
        else
            rmdir $PRESERV_DIR
        fi
    fi
    echo
    echo "#### Error on cmake project configuration phase. Exiting. ####"
    exit 1
fi

# Cleans-up the installation dir, restoring any previously existing device,
# plugin and all contents under var.
rm -rf $FRAMEWORK_INSTALL_DIR
mkdir -p $FRAMEWORK_INSTALL_DIR
if [[  -n $PRESERV_DIR && -d $PRESERV_DIR ]]; then
    mv $PRESERV_DIR/* $FRAMEWORK_INSTALL_DIR
    if [ $? -ne 0 ]; then
        echo "WARNING: Failed to restore content from \"$PRESERVE_DIR\" into \"$FRAMEWORK_INSTALL_DIR\"."
        echo "         Please try to restore it manually."
    else
        rmdir $PRESERV_DIR
    fi
fi

# Builds libkarabo, libkarathon, karabo-* utilities and install them
# in FRAMEWORK_INSTALL_DIR.
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
    source $FRAMEWORK_BUILD_DIR/activateKarabo.sh
    # When GEN_CODE_COVERAGE is true, the cmake configuration phase generates
    # a 'test_coverage_report' target, than when built runs the tests and
    # generates the coverage report.
    $EXTERN_DEPS_DIR/bin/cmake --build . -j $NUM_JOBS --target test_coverage_report
    deactivateKarabo
    # Generate the coverage report for the Python tests - all of them.
    producePythonCodeCoverageReport
    exit 0
fi

if [ "$BUILD_UNIT_TESTING" = "1" ] || [ "$BUILD_INTEGRATION_TESTING" = "1" ] || [ "$BUILD_LONG_RUN_TESTING" = "1" ]; then
    echo
    echo Running Karabo C++ tests ...
    echo
    # Activate the karabo environment to allow tests to be run from the
    # build tree.
    source $FRAMEWORK_BUILD_DIR/activateKarabo.sh
    safeRunCommand ctest -VV
    deactivateKarabo
fi

if [ "$RUNTESTS" = "y" ]; then
    runPythonUnitTests
fi

if [ "$RUNINTEGRATIONTESTS" = "y" ]; then
    runPythonIntegrationTests
fi

if [ "$RUNLONGTESTS" = "y" ]; then
    runPythonLongTests
fi

echo "### Successfully finished building and packaging of karaboFramework ###"
echo
exit 0
