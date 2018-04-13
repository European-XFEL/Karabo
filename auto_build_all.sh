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
    if [ -z "$KARABO" ]; then
        source $scriptDir/karabo/activate
    fi

    local testNames="AlarmService_ Device_ LockTest_ PipelinedProcessing_ PropertyTest_ RunTimeSchemaAttributes_ SceneProvider_ Timing_"
    local testDir=$scriptDir/build/netbeans/integrationTests

    echo
    echo Running Karabo integration tests ...
    echo
    cd $testDir
    rm -rf testresults/
    safeRunCommand "make CONF=$CONF -j$NUM_JOBS test"
    cd $scriptDir

    # Parse the XML test outputs
    checkCppUnitTestResults "$testDir" "$testNames"

}

runUnitTests() {
    if [ -z "$KARABO" ]; then
        source $scriptDir/karabo/activate
    fi

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

    echo
    echo Running pythonKarabo tests
    echo
    cd build/netbeans/pythonKarabo
    safeRunCommand "nosetests -v karabo.bound_api"
    safeRunCommand "nosetests -v karabo.common"
    safeRunCommand "nosetests -v karabo.project_db"
    safeRunCommand "nosetests -v karabo.tests"
    safeRunCommand "nosetests -v karabo_gui"
    safeRunCommand "nosetests -v karabogui"
    safeRunCommand "nosetests -v karabo.interactive"
    safeRunCommand "nosetests -v karabo.usermacro_api"
    safeRunCommand "nosetests -v karabo.middlelayer_api"

    echo
    echo Unit tests complete
    echo
}

runPythonIntegrationTests() {
    if [ -z "$KARABO" ]; then
        source $scriptDir/karabo/activate
    fi

    echo
    echo Running Karabo Python integration tests ...
    echo
    cd $scriptDir/src/pythonKarabo/karabo/integration_tests/
    cd device_comm_test
    safeRunCommand "python3 -m unittest discover -v"
    cd ..
    cd device_provided_scenes_test
    safeRunCommand "python3 -m unittest discover -v"
    cd ..
    cd run_configuration_group
    safeRunCommand "python3 -m unittest discover -v"
    cd ..
    cd device_cross_test
    safeRunCommand "python3 -m unittest discover -v"
    cd ..
    echo
    echo Integration tests complete
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
Usage: $0 Debug|Release|Dependencies|Clean|Clean-All [flags]

Available flags:
  --auto       - Tries to automatically install needed system packages (sudo rights required!)
  --bundle     - Installs Karabo and creates the software bundle. Default: no bundle is created!
  --pyDevelop  - Install Python packages in development mode
  --runTests   - Run unit tests after building (useful for Debug|Release)
  --runIntegrationTests
               - Run integration tests after building (for Debug|Release)
  --numJobs N  - Specify the number of jobs that make should use to run simultaneously

Note: "Dependencies" builds only the external dependencies
      "Clean" cleans all Karabo code (src folder)
      "Clean-All" additionally cleans all external dependencies (extern folder)

End-of-help

    exit 0
fi

EXTERN_ONLY="n"

# Fetch configuration type (Release or Debug)
if [[ $1 = "Release" || $1 = "Debug" ]]; then
    CONF=$1
elif [[ $1 = "Clean" || $1 = "Clean-All" ]]; then
    safeRunCommand "cd $scriptDir/build/netbeans/karabo"
    safeRunCommand "make bundle-clean CONF=Debug"
    safeRunCommand "make bundle-clean CONF=Release"
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
    echo "Invalid option supplied. Allowed options: Release|Debug|Dependencies|Clean|Clean-All"
    echo
    exit 1
fi

# Get rid of the first argument
shift

# Parse the commandline flags
BUNDLE="n"
RUNTESTS="n"
RUNINTEGRATIONTESTS="n"
PYOPT="normal"
NUM_JOBS=0
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

# Get some information about our system
OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    if [ "$NUM_JOBS" = "0" ]; then
        NUM_JOBS=`grep "processor" /proc/cpuinfo | wc -l`
    fi
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    if [ "$NUM_JOBS" = "0" ]; then
        NUM_JOBS=`sysctl hw.ncpu | awk '{print $2}'`
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
    runIntegrationTests # temporarily...
    runUnitTests
fi

if [ "$RUNINTEGRATIONTESTS" = "y" ]; then
    runIntegrationTests
    runPythonIntegrationTests
fi


echo "### Successfully finished building and packaging of karaboFramework ###"
echo
exit 0
