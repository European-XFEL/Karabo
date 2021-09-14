#!/bin/bash

# WARNING: This script needs to be run in a proper environment.
# The karabo activate file needs to be sourced and a broker needs to be
# started.

# This variables are created to ensure that 'nosetest' and 'coverage'
# are executed from the karabo environment.
COVERAGE="python $(which coverage)"
# For some tests we allow flakiness:
FLAKY_FLAGS="--with-flaky --no-success-flaky-report"

# coverage configuration file
COVERAGE_CONF_FILE=".coveragerc"

# A directory to which the code coverage will be stored.
CODE_COVERAGE_DIR_NAME="pyReport"

# A flag that indicates if sitecustomize.py file was created.
SITE_CUSTOMIZE_FILE_CREATED=false

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

# Set-up coverage tool to also collect coverage data in sub-processes.
setupCoverageTool() {
    echo "Entering setupCoverageTool."

    # Set coverage variables.
    COVER_COVERED_PACKAGES=""
    COVER_COVERED_PACKAGES+="--cover-package=karabo "
    # MR-3871: disable while karabogui doesn't support Qt5 on the old deps
    #COVER_COVERED_PACKAGES+="--cover-package=karabogui "
    COVER_COVERED_PACKAGES+="--cover-inclusive"
    COVER_FLAGS="--with-coverage $COVER_COVERED_PACKAGES"

    # Path to the sitecustomize.py file.
    SITE_PACKAGES_DIR=$KARABO_PROJECT_ROOT_DIR/karabo/extern/bin/python -c 'import site; print(site.getsitepackages()[0])'
    SITE_CUSTOMIZE_FILE_PATH=$SITE_PACKAGES_DIR/sitecustomize.py

    SITE_CUSTOMIZE_FILE_CREATED=false

    # Create '.coveragerc' file.
    echo "
[run]
  source = $KARABO_PROJECT_ROOT_DIR/karabo
  parallel = True
  data_file = $(pwd)/.coverage
" > $COVERAGE_CONF_FILE

    if [ ! -f $SITE_CUSTOMIZE_FILE_PATH ]; then
        # Create the sitecustomize.py script.

        SITE_CUSTOMIZE_FILE_CREATED=true

        echo "Creating the sitecustomize.py file. path = '$SITE_CUSTOMIZE_FILE_PATH'"

        echo "import coverage" > $SITE_CUSTOMIZE_FILE_PATH
        echo "coverage.process_startup()" >> $SITE_CUSTOMIZE_FILE_PATH
    fi

    # Enable coverage to collect the code coverage data from all the processes
    # in the following tests.
    export COVERAGE_PROCESS_START=$(pwd)/.coveragerc
    # Instruct 'coverage' to display a message when creating a data file.
    # This is extremely useful when debugging problems related to the
    # multi-process code coverage collecting process.
    # export COVERAGE_DEBUG=dataio
}

# Tear-down the configurations made by the 'setupCoverageTool' function.
teardownCoverageTool() {
    if $SITE_CUSTOMIZE_FILE_CREATED; then

        echo "Removing the sitecustomize.py file. path = '$SITE_CUSTOMIZE_FILE_PATH'"

        # Remove the sitecustomize.py file if created by this script.
        rm $SITE_CUSTOMIZE_FILE_PATH
    fi

    unset SITE_CUSTOMIZE_FILE_CREATED

    # Disable 'coverage' data collection.
    unset COVERAGE_PROCESS_START
    # Disable 'coverage' logging.
    unset COVERAGE_DEBUG
}

onExit() {
    teardownCoverageTool
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
            onExit
            exit $ret_code
        fi
        # Append failed command and increase counter
        FAILED_TESTS+="'$cmnd'\n"
        ((NUM_FAILED_TESTS+=1))
    fi
}

safeRunTests() {
    # mandatory argument: the python module specification
    # optional argument: extra options
    # optional argument: output_xml_filename
    MODULE_SPEC=$1
    JUNIT_OUTPUT=""
    if $COLLECT_COVERAGE; then
        safeRunCommand "python $(which nosetests) -v $COVER_FLAGS $FLAKY_FLAGS $MODULE_SPEC"
    else
        _OLD_ACCEPT=$ACCEPT_FAILURES
        ACCEPT_FAILURES=true
        PYTHON_TESTS="py.test -v --pyargs"
        JUNIT_RESULT_FILE="junit.${MODULE_SPEC}${2}.xml"
        rm -f $JUNIT_RESULT_FILE
        JUNIT_OUTPUT="--junitxml=$JUNIT_RESULT_FILE"
        safeRunCommand "$PYTHON_TESTS $JUNIT_OUTPUT $MODULE_SPEC"
        ACCEPT_FAILURES=$_OLD_ACCEPT
        unset _OLD_ACCEPT
    fi
}

runPythonUnitTests() {

    savedBroker=${KARABO_BROKER}
    export KARABO_BROKER="tcp://exflbkr02n0:7777"

    echo
    echo
    echo "*************************************************************************************************"
    echo "*************************************************************************************************"
    echo "**"
    echo "**   Running Karabo Python unit tests with JMS broker  ... $KARABO_BROKER"
    echo "**"
    echo "*************************************************************************************************"
    echo "*************************************************************************************************"
    echo
    echo

    safeRunTests "karabo.bound_api" "_jms"
    safeRunTests "karabo.bound_devices" "_jms"
    safeRunTests "karabo.middlelayer_api" "_jms"
    safeRunTests "karabo.middlelayer_devices" "_jms"
    safeRunTests "karabo.common" "_jms"
    safeRunTests "karabo.macro_api" "_jms"
    safeRunTests "karabo.macro_devices" "_jms"
    safeRunTests "karabo.influxdb" "_jms"
    safeRunTests "karabo.native" "_jms"
    safeRunTests "karabo.project_db" "_jms"
    safeRunTests "karabo.config_db" "_jms"
    safeRunTests "karabo.tests" "_jms"
    safeRunTests "karabo.interactive" "_jms"

    export KARABO_BROKER="mqtt://exfldl02n0:1883"

    echo
    echo
    echo "*************************************************************************************************"
    echo "*************************************************************************************************"
    echo "**"
    echo "**   Running Karabo Python unit tests with MQTT broker ... $KARABO_BROKER"
    echo "**"
    echo "*************************************************************************************************"
    echo "*************************************************************************************************"
    echo
    echo

    safeRunTests "karabo.bound_api" "_mqtt"
    safeRunTests "karabo.bound_devices" "_mqtt"
    safeRunTests "karabo.middlelayer_api" "_mqtt"
    safeRunTests "karabo.middlelayer_devices" "_mqtt"
    safeRunTests "karabo.common" "_mqtt"
    safeRunTests "karabo.macro_api" "_mqtt"
    safeRunTests "karabo.macro_devices" "_mqtt"
    safeRunTests "karabo.influxdb" "_mqtt"
    safeRunTests "karabo.native" "_mqtt"
    safeRunTests "karabo.project_db" "_mqtt"
    safeRunTests "karabo.config_db" "_mqtt"
    safeRunTests "karabo.tests" "_mqtt"
    safeRunTests "karabo.interactive" "_mqtt"

#     export KARABO_BROKER="amqp://xfel:karabo@exflctrl01:5672"
#
#     echo
#     echo
#     echo "*************************************************************************************************"
#     echo "*************************************************************************************************"
#     echo "**"
#     echo "**   Running Python unit tests with RabbitMQ (AMQP) broker ... $KARABO_BROKER"
#     echo "**"
#     echo "*************************************************************************************************"
#     echo "*************************************************************************************************"
#     echo
#     echo
#
#     safeRunTests "karabo.bound_api" "_amqp"
#     safeRunTests "karabo.bound_devices" "_amqp"
#     safeRunTests "karabo.middlelayer_api" "_amqp"
#     safeRunTests "karabo.middlelayer_devices" "_amqp"
#     safeRunTests "karabo.common" "_amqp"
#     safeRunTests "karabo.macro_api" "_amqp"
#     safeRunTests "karabo.macro_devices" "_amqp"
#     safeRunTests "karabo.influxdb" "_amqp"
#     safeRunTests "karabo.native" "_amqp"
#     safeRunTests "karabo.project_db" "_amqp"
#     safeRunTests "karabo.config_db" "_amqp"
#     safeRunTests "karabo.tests" "_amqp"
#     safeRunTests "karabo.interactive" "_amqp"

    export KARABO_BROKER="redis://exflctrl01:6379"

    echo
    echo
    echo "*************************************************************************************************"
    echo "*************************************************************************************************"
    echo "**"
    echo "**   Running Python unit tests with REDIS broker (server) ... ${KARABO_BROKER}"
    echo "**"
    echo "*************************************************************************************************"
    echo "*************************************************************************************************"
    echo
    echo

    safeRunTests "karabo.bound_api" "_redis"
    safeRunTests "karabo.bound_devices" "_redis"
    safeRunTests "karabo.middlelayer_api" "_redis"
    safeRunTests "karabo.middlelayer_devices" "_redis"
    safeRunTests "karabo.common" "_redis"
    safeRunTests "karabo.macro_api" "_redis"
    safeRunTests "karabo.macro_devices" "_redis"
    safeRunTests "karabo.influxdb" "_redis"
    safeRunTests "karabo.native" "_redis"
    safeRunTests "karabo.project_db" "_redis"
    safeRunTests "karabo.config_db" "_redis"
    safeRunTests "karabo.tests" "_redis"
    safeRunTests "karabo.interactive" "_redis"

    export KARABO_BROKER=${savedBroker}

    echo
    echo Karabo Python unit tests complete
    echo
}

runPythonIntegrationTests() {
    echo
    echo Running Karabo Python integration tests ...
    echo

    # TODO: Needs to be uncommented when the bound_device_test integration test is added.
    #safeRunTests "karabo.integration_tests.bound_device_test"
    safeRunTests "karabo.integration_tests.all_api_alarm_test"
    safeRunTests "karabo.integration_tests.config_manager_cross_test"
    safeRunTests "karabo.integration_tests.device_comm_test"
    safeRunTests "karabo.integration_tests.device_cross_test"
    safeRunTests "karabo.integration_tests.device_provided_scenes_test"
    safeRunTests "karabo.integration_tests.device_schema_injection_test"
    safeRunTests "karabo.integration_tests.pipeline_processing_test"
    safeRunTests "karabo.integration_tests.signal_slot_order_test"
    echo
    echo Karabo Python integration tests complete
    echo
}

runPythonLongTests() {
    echo
    echo Running Karabo Python long tests ...
    echo

    safeRunTests "karabo.integration_tests.pipeline_cross_test"

    echo
    echo Karabo Python long tests complete
    echo
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
    # MR-3871: disable while karabogui doesn't support Qt5 on the old deps
    #safeRunCommand $COVERAGE html -i --include "*/karabogui/*" --omit $OMIT -d htmlcov_karabogui
    safeRunCommand $COVERAGE html -i --include "*/site-packages/karabo/bound_devices/*" --omit $OMIT -d htmlcov_bound_devices

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

    # Remove coverage configuration file file.
    rm -rf $COVERAGE_CONF_FILE

    # Remove code coverage directory if it exists.
    rm -rf $CODE_COVERAGE_DIR_PATH
}

# Main

# Parse arguments.

RUN_CONDA_UNIT_TEST=false
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

if $COLLECT_COVERAGE; then
    # Prepare coverage tool.

    # Set-up coverage tool.
    setupCoverageTool
fi

if $RUN_UNIT_TEST; then
    runPythonUnitTests
fi

if $RUN_INTEGRATION_TEST; then
    runPythonIntegrationTests
fi

if $RUN_LONG_TEST; then
    runPythonLongTests
fi


if $COLLECT_COVERAGE; then
    # Clean up after coverage tool.

    # Tear-down configuration for coverage tool.
    teardownCoverageTool
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
