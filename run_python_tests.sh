#!/bin/bash

# WARNING: This script needs to be run in a proper environment.
# The karabo activate file needs to be sourced and a broker needs to be
# started.

# This variables are created to ensure that 'coverage'
# is executed from the karabo environment.
COVERAGE="python $(which coverage)"
# For some tests we allow flakiness:
FLAKY_FLAGS="--with-flaky --no-success-flaky-report"

# A directory to which the code coverage will be stored.
CODE_COVERAGE_DIR_NAME="pyReport"

ACCEPT_FAILURES=false
FAILED_TESTS=
typeset -i NUM_FAILED_TESTS=0

displayHelp()
{
    echo "
Usage: $0 [flags]

Needs an activated Karabo environment.

Available flags:
  --help, -h - Display help.
  --rootDir <path> - Path to the root directory of the Karabo project.
  --runUnitTests - Run Python unit tests.
  --runIntegrationTests - Run Python integration tests.
  --runLongTests - Run Python long tests
  --collectCoverage - Collect Python code coverage.
  --generateCoverageReport - Collect Python code coverage and generate coverage
        report.
  --reportDir - A path where the code coverage report will be stored. If the
        flag is not present, the report will be stored in project's root
        directory (--rootDir).
  --clean - Remove all the code coverage information collected in the previous
        tests.
  --force - Go on even if some tests fail. The script's return code will
            indicate how many test commands failed.

What combination of flags you usually want to use:
$ ./run_python_tests.sh --clean --runUnitTests --runIntegrationTests \\
$       --collectCoverage --generateCoverageReport --rootDir <path>
This will run the tests, collect the code coverage, generate a html code
coverage report and store it to '<root_dir_path>/$CODE_COVERAGE_DIR_NAME'
directory.

The --rootDir <path> needs to point to a root directory of Karabo project. The
provided path is used to find files needed by the code coverage tools.

To specify where the report should be stored, the '--reportDir <path>' flag
can be used. If the flag is not present, the report will be stored to
the root directory of Karabo project.

The --runUnitTests and --runIntegrationTests flags only run the unit and
integration tests. So, if you want to only run the tests, run:
$ ./run_python_tests.sh --runUnitTests --runIntegrationTests --rootDir <path>

To collect the code coverage, the --collectCoverage flag can be used.
$ ./run_python_tests.sh --clean --runUnitTests --runIntegrationTests \\
$       --collectCoverage --rootDir <path>
This command will run the tests, collect the code coverage and store the
code coverage into a .coverage file in the directory where the script was
executed. This command might come in handy if you want to generate a coverage
report with a specific tool and for that you need only the .coverage file.

The --clean flag is used to remove all the previously collected code coverage
data. Usually, you want to use this every time when collecting the code
coverage because you wan to have a clean start (i.e. no data from the previous
cases when you collected the code coverage), but there are cases when you do
not want to do that. For example: When you writing another script that collect
the code coverage data in several places and needs to be a bit more dynamic.


The order in which you call the flags is not important. If a flag is repeated,
the script behaves as it would if the flag was only called once.
"
}

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
        if ! $ACCEPT_FAILURES; then
            exit $ret_code
        fi
        # Append failed command and increase counter
        FAILED_TESTS+="'$cmnd'\n"
        ((NUM_FAILED_TESTS+=1))
    fi
}

safeRunTests() {
    # mandatory argument: the name of the broker "technology"
    # mandatory argument: the address of the broker
    # mandatory argument: the name of the tests
    # mandatory argument: the python module specification
    # optional argument: extra options
    BROKER=$1
    TEST_SUITE_NAME=$2
    BROKER_TYPE=$(echo $BROKER | sed -E "s|(.+)://(.+)|\1|g")
    if [ "${BROKER_TYPE}" == "tcp" ]; then
        BROKER_TYPE="jms"
    fi

    if [ "${BROKER_TYPE}" == "amqp" -a "${TEST_SUITE_NAME}" == "Integration" ]; then
        echo "***************************************************************************************"
        echo "**"
        echo "** For now skip ${TEST_SUITE_NAME} tests for ${BROKER_TYPE} broker"
        echo "** (Revive by editing 'run_python_tests.sh'.)"
        echo "**"
        echo "***************************************************************************************"
        return
    fi

    export KARABO_BROKER=$BROKER
    echo
    echo "*************************************************************************************************"
    echo "*************************************************************************************************"
    echo "**"
    echo "**   Running Karabo Python ${TEST_SUITE_NAME} tests with ${BROKER_TYPE^^} broker  ... ${KARABO_BROKER}"
    echo "**"
    echo "*************************************************************************************************"
    echo "*************************************************************************************************"
    echo
    echo

    MODULE_SPEC=$3
    EXTRA_FLAGS=$4
    FLAGS="--pyargs"
    if [ -n "${EXTRA_FLAGS}" ]; then
        FLAGS="${FLAGS} ${EXTRA_FLAGS}"
    fi
    if $COLLECT_COVERAGE; then
        FLAGS="${FLAGS} --cov=karabo"
    fi
    _OLD_ACCEPT=$ACCEPT_FAILURES
    ACCEPT_FAILURES=true
    PYTHON_TESTS="py.test -v ${FLAGS}"
    JUNIT_RESULT_FILE="junit.${TEST_SUITE_NAME}.${MODULE_SPEC}_${BROKER_TYPE}.xml"
    rm -f $JUNIT_RESULT_FILE
    JUNIT_OUTPUT="--junitxml=$JUNIT_RESULT_FILE"
    safeRunCommand "$PYTHON_TESTS $JUNIT_OUTPUT $MODULE_SPEC"
    ACCEPT_FAILURES=$_OLD_ACCEPT
    unset _OLD_ACCEPT
}

runPythonTestsCI() {
    IFS=';' read -r -a BROKERS <<< "$KARABO_CI_BROKERS"
    for BROKER in "${BROKERS[@]}"
    do
        safeRunTests $BROKER $*
    done
}

runPythonTests() {
    savedBroker=${KARABO_BROKER}

    if [ ! -z "$KARABO_CI_BROKERS" ]; then
        runPythonTestsCI $*
    else
        # Uncomment or adjust depending on the broker environment
        safeRunTests "tcp://exflbkr02n0:7777" $*
        # safeRunTests "mqtt://exfldl02n0:1883" $*
        # safeRunTests "amqp://xfel:karabo@exflctrl01:5672" $*
        # safeRunTests "redis://exflctrl01:6379" $*
    fi

    export KARABO_BROKER=${savedBroker}

    echo
    echo Karabo Python $1 tests complete
    echo
}

runPythonUnitTests() {
    runPythonTests "Unit" "karabo" "--ignore-glob=*integration_tests*"
}

runPythonIntegrationTests() {
    runPythonTests "Integration" "karabo.integration_tests" "--ignore-glob=*karabo.integration_tests.pipeline_cross_test*"
}

runPythonCoverageTests() {
    runPythonTests "All" "karabo" "--ignore-glob=*karabo.integration_tests.pipeline_cross_test*"
}

runPythonLongTests() {
    runPythonTests "Long" "karabo.integration_tests.pipeline_cross_test"
}

generateCodeCoverageReport() {
    # What to omit when generating html coverage reports.
    OMIT="*/site-packages/karabo/*tests*","*/site-packages/karabo/*_test.py"

    echo
    echo Generating HTML coverage reports ...
    echo

    # Combine all the data files.
    $COVERAGE combine

    # Remove code coverage directory if it exists.
    rm -rf $CODE_COVERAGE_DIR_PATH

    # Generate coverage for the following modules.
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_karabo"
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/bound_api/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_bound_api"
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/middlelayer_api/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_middlelayer_api"
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/common/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_common"
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/project_db/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_project_db"
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/config_db/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_config_db"
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/interactive/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_interactive"
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/bound_devices/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_bound_devices"
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/middlelayer_devices/*" --omit $OMIT -d "$CODE_COVERAGE_DIR_PATH/htmlcov_middlelayer_devices"

    echo
    echo HTML coverage reports generation complete
    echo

    # Zip the report.

    TIMESTAMP=$(date +%F_%H%M%S)
    ZIP_FILE=$CODE_COVERAGE_DIR_NAME'_'$TIMESTAMP'.zip'

    PREV_DIR=$(pwd)
    cd $CODE_COVERAGE_BASE_DIR

    safeRunCommand zip -q -r $ZIP_FILE $CODE_COVERAGE_DIR_NAME
    safeRunCommand mv $ZIP_FILE $CODE_COVERAGE_DIR_NAME

    cd $PREV_DIR

    echo
    echo "### The Python coverage report can be found at:"
    echo "### $CODE_COVERAGE_DIR_PATH/htmlcov_karabo/index.html"
    echo "### or in zipped form at: $CODE_COVERAGE_DIR_PATH/$ZIP_FILE."
    echo
}

# Clean the code coverage report and coverage related files.
clean() {
    echo
    echo Cleaning the code coverage report and coverage related files
    echo

    # Clean the previous coverage date.
    $COVERAGE erase
}

# Main

# Parse arguments.

RUN_UNIT_TEST=false
RUN_INTEGRATION_TEST=false
RUN_LONG_TEST=false
COLLECT_COVERAGE=false
GENERATE_COVERAGE_REPORT=false
DISPLAY_HELP=false
CLEAN=false

if (( $# == 0 )); then
    DISPLAY_HELP=true
else
    while [ -n "$1" ]; do
        case "$1" in
            --runUnitTests)
                RUN_UNIT_TEST=true
                ;;
            --runIntegrationTests)
                RUN_INTEGRATION_TEST=true
                ;;
            --runLongTests)
                RUN_LONG_TEST=true
                ;;
            --collectCoverage)
                COLLECT_COVERAGE=true
                ;;
            --generateCoverageReport)
                GENERATE_COVERAGE_REPORT=true
                ;;
            --clean)
                CLEAN=true
                ;;
            --force)
                ACCEPT_FAILURES=true
                ;;
            --rootDir)
                if ! [ -n "$2" ]; then
                    echo "Error: The '--rootDir' flag was used but the path was not specified."
                    echo "Specify the path."

                    exit 1
                fi

                KARABO_PROJECT_ROOT_DIR=$2
                shift
                ;;
            --reportDir)
                if ! [ -n "$2" ]; then
                    echo "Error: The '--reportDir' flag was used but the path was not specified."
                    echo "Specify the path."

                    exit 1
                fi

                CODE_COVERAGE_BASE_DIR=$2
                shift
                ;;

            --help)
                DISPLAY_HELP=true
                ;;
            -h)
                DISPLAY_HELP=true
                ;;
            *)
                echo "Unrecognized commandline flag: $1"
                exit 1
                ;;
        esac
        shift
    done
fi

if $DISPLAY_HELP; then
    displayHelp
    exit 1
fi

if [ -z $KARABO_PROJECT_ROOT_DIR ]; then
    echo "Error: The path to the root directory of Karabo project was not specified."
    echo "Use the '--rootDir <path>' flag."
    echo ""
    exit 1
fi

if [ -z $CODE_COVERAGE_BASE_DIR ]; then
    # If the --reportDir flag is not specified, then project's root directory is used.
    CODE_COVERAGE_BASE_DIR=$KARABO_PROJECT_ROOT_DIR
fi
CODE_COVERAGE_DIR_PATH=$CODE_COVERAGE_BASE_DIR/$CODE_COVERAGE_DIR_NAME

if $CLEAN; then
    clean
fi

# Run tests.

if $RUN_UNIT_TEST; then
    runPythonUnitTests
fi

if $RUN_INTEGRATION_TEST; then
    runPythonIntegrationTests
fi

if $RUN_LONG_TEST; then
    runPythonLongTests
fi

# Generate report.

if $GENERATE_COVERAGE_REPORT; then
    # Sleep for X seconds so that the 'coverage' is able to create data files.
    sleep 5

    generateCodeCoverageReport
fi

if [ $NUM_FAILED_TESTS -gt 0 ] ; then
    echo
    echo "Following $NUM_FAILED_TESTS commands failed:"
    echo
    echo -e $FAILED_TESTS  # -e to interpete \n as newline

    exit $NUM_FAILED_TESTS
fi
