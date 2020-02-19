#!/bin/bash

# Script for automated building and packaging of the entire karaboFramework
#
# Author: <burkhard.heisen@xfel.eu>
#

# Help function for checking successful execution of commands
safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    echo cmnd=$cmnd
    eval $cmnd
    ret_code=$?
    if [ $ret_code != 0 ]; then
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        echo
        echo
        exit $ret_code
    fi
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
    for name in $testNames; do
        local path=$(printf "%s/testresults/%sTest.xml" $testDir $name)
        if [ ! -e $path ]; then
            echo "Expected test results file: $path not found!"
            echo
            echo
            exit 1
        fi
        # xmllint is distributed with Karabo
        local fails=$(xmllint --xpath '/TestRun/Statistics/FailuresTotal/text()' $path)
        if [ $fails != "0" ]; then
            echo "Test failure found in results file: $path"
            echo
            echo
            exit 1
        fi
    done

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

runIntegrationTests() {
    activateKarabo

    local testNames="AlarmService_ DataLogging_ Device_ GuiVersion_ LockTest_ PipelinedProcessing_ PropertyTest_ RunTimeSchemaAttributes_ SceneProvider_ Timing_"
    local testDir=$scriptDir/build/netbeans/integrationTests

    echo
    echo Running Karabo C++ integration tests ...
    echo
    cd $testDir
    rm -rf testresults/
    safeRunCommand "make CONF=$CONF -j$NUM_JOBS test"
    cd $scriptDir

    # Parse the XML test outputs
    checkCppUnitTestResults "$testDir" "$testNames"

}

runCppLongTests() {
    activateKarabo

    local testNames=$(ls $scriptDir/src/cppLongTests)
    local testDir=$scriptDir/build/netbeans/cppLongTests

    echo
    echo Running Karabo C++ long tests ...
    echo
    cd $testDir
    rm -rf testresults/
    safeRunCommand "make CONF=$CONF -j$NUM_JOBS test"
    cd $scriptDir

    # Parse the XML test outputs
    checkCppUnitTestResults "$testDir" "$testNames"

}

runUnitTests() {
    activateKarabo
    local testNames=$(ls $scriptDir/src/karabo/tests)
    local testDir=$scriptDir/build/netbeans/karabo

    echo
    echo Running Karabo unit tests ...
    echo
    cd $testDir
    rm -rf testresults/
    safeRunCommand "make CONF=$CONF -j$NUM_JOBS test"
    cd $scriptDir

    # Parse the XML test outputs
    checkCppUnitTestResults "$testDir" "$testNames"

    #
    # Running pythonKarabo tests
    #

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

    #
    # Running Karabo Python integration tests ...
    #

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

produceCodeCoverageReport() {
    echo "### Producing code coverage reports..."
    echo

    # Needed for 'run_python_tests.sh --clean ...':
    activateKarabo

    # remove any previous code coverage results
    safeRunCommand "find . -name \"*.gcda\" -delete"
    safeRunCommand $scriptDir/run_python_tests.sh --clean --rootDir $scriptDir

    runUnitTests
    runIntegrationTests
    runPythonIntegrationTests
    runCppLongTests
    runPythonLongTests

    # produce initial C++ coverage information
    safeRunCommand "$scriptDir/ci/coverage/report/gen_initial"

    safeRunCommand "$scriptDir/ci/coverage/report/gen_report"

    # Most recent zip file - there mightbe others from previous runs
    local ZIP_FILE_NAME=`ls -t ./ci/coverage/report/*.zip | head -1`

    # produce initial Python coverage information
    safeRunCommand $scriptDir/run_python_tests.sh \
        --generateCoverageReport \
        --rootDir $scriptDir \
        --reportDir $scriptDir/ci/coverage

    echo
    echo "### The C++ coverage results can be found at:"
    echo "### $scriptDir/ci/coverage/report/out/index.html"
    echo "### or in zipped form: $ZIP_FILE_NAME"
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
Usage: $0 Debug|Release|CodeCoverage|Dependencies|Clean|Clean-All [flags]

Available flags:
  --bundle     - Installs Karabo and creates the software bundle. Default: no bundle is created!
  --pyDevelop  - Install Python packages in development mode
  --runTests   - Run unit tests after building (useful for Debug|Release)
  --runIntegrationTests
               - Run integration tests after building (for Debug|Release)
  --runLongTests
               - Run long running tests after building (for Debug|Release)
  --numJobs N  - Specify the number of jobs that make should use to run simultaneously

Note: "Dependencies" builds only the external dependencies
      "Clean" cleans all Karabo code (src folder)
      "Clean-All" additionally cleans all external dependencies (extern folder)
      "CodeCoverage" builds the Karabo framework with CodeCoverage configuration,
                     but also implicitly runs the unit, integration and long tests
                     and produces code coverage reports. The CodeCoverage configuration
                     also disables --pyDevelop option.
      The environment variable "KARABO_TEST_BROKER" can be set to force the unit
      tests to run on the broker set in this variable.
      Please note that tests will activate the Karabo installation currently
      built.
End-of-help

    exit 0
fi

EXTERN_ONLY="n"

# Fetch configuration type (Release or Debug)
if [[ $1 = "Release" || $1 = "Debug" || $1 = "CodeCoverage" ]]; then
    CONF=$1
elif [[ $1 = "Clean" || $1 = "Clean-All" ]]; then
    safeRunCommand "cd $scriptDir/build/netbeans/karabo"
    safeRunCommand "make bundle-clean CONF=Debug"
    safeRunCommand "make bundle-clean CONF=Release"
    safeRunCommand "make bundle-clean CONF=CodeCoverage"
    if [[ $1 = "Clean-All" ]]; then
        safeRunCommand "make clean-extern"
    fi
    safeRunCommand "cd $scriptDir/build/netbeans/karabo"
    rm -rf dist build nbproject/Makefile* nbproject/Package* nbproject/private
    safeRunCommand "cd $scriptDir/build/netbeans/karathon"
    rm -rf dist build nbproject/Makefile* nbproject/Package* nbproject/private
    safeRunCommand "cd $scriptDir/build/netbeans/deviceServer"
    rm -rf dist build nbproject/Makefile* nbproject/Package* nbproject/private
    safeRunCommand "cd $scriptDir/build/netbeans/brokerMessageLogger"
    rm -rf dist build nbproject/Makefile* nbproject/Package* nbproject/private
    exit 0
elif [[ $1 = "Dependencies" ]]; then
    echo "Building external dependencies"
    EXTERN_ONLY="y"
else
    echo
    echo "Invalid option supplied. Allowed options: Release|Debug|CodeCoverage|Dependencies|Clean|Clean-All"
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
        *)
            # Make a little noise
            echo "Unrecognized commandline flag: $1"
            ;;
    esac
    shift
done

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


# Get some information about our system
OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    if [ "$NUM_JOBS" = "0" ]; then
        NUM_JOBS=`grep "processor" /proc/cpuinfo | wc -l`
    fi
fi

echo
echo "### Starting compilation (using $NUM_JOBS jobs) and packaging of the karaboFramework. ###"
echo

sleep 2

safeRunCommand "cd $scriptDir/build/netbeans/karabo"

if [ $EXTERN_ONLY = "y" ]; then
    safeRunCommand "make -j$NUM_JOBS extern"
    if [ "$BUNDLE" = "y" ]; then
        safeRunCommand "make -j$NUM_JOBS package-extern"
    fi
elif [ "$BUNDLE" = "y" ]; then
    safeRunCommand "make CONF=$CONF PYOPT=$PYOPT -j$NUM_JOBS bundle-package"
else
    safeRunCommand "make CONF=$CONF PYOPT=$PYOPT -j$NUM_JOBS bundle-install"
fi

if [ "$RUNTESTS" = "y" ]; then
    runUnitTests
fi

if [ "$RUNINTEGRATIONTESTS" = "y" ]; then
    runIntegrationTests
    runPythonIntegrationTests
fi

if [ "$RUNLONGTESTS" = "y" ]; then
    runCppLongTests
    runPythonLongTests
fi

if [ "$CODECOVERAGE" = "y" ]; then
    produceCodeCoverageReport
fi

echo "### Successfully finished building and packaging of karaboFramework ###"
echo
exit 0
