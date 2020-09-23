#!/bin/bash

# WARNING: This script needs to be run in a proper environment.
# The karabo activate file needs to be sourced and a broker needs to be
# started.

# This variables are created to ensure that 'nosetest' and 'coverage'
# are executed from the karabo environment.
COVERAGE="python $(which coverage)"
NOSETESTS="python $(which nosetests)"
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
  --runCondaUnitTests - Run the conda python unit tests (gui only)
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
    SITE_PACKAGES_DIR=$KARABO_PROJECT_ROOT_DIR/karabo/extern/lib/python3.6/site-packages
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
    typeset -i max_tries=4

    echo cmnd=$cmnd
    eval $cmnd
    ret_code=$?
    if $ACCEPT_SIGSEGV; then
        # the default return code of a process segfaulting is 139
        # which is the "abnormal termination" code 128 plus the signal number (SIGSEGV=11)
        while [ $max_tries -gt 1 ] && [ $ret_code == 139 ]; do
            ((max_tries--))
            printf "Segmentation Fault: [%d] when executing command: '$cmnd' - %d tries left" $ret_code $max_tries
            eval $cmnd
            ret_code=$?
        done
    fi
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

runCondaUnitTests() {
    # Setup the environment
    safeRunCommand source ./build_conda_env.sh clean install

    PLATFORM=$(python -m platform)
    if [[ $PLATFORM == Darwin* ]]; then
        # TODO: figure out why the GUI tests fail on MacOs
        ACCEPT_SIGSEGV=true
        safeRunCommand "nosetests -v $COVER_FLAGS -e 'test_basics|test_set_value|test_decline_color|test_property_proxy_edit_values_from_text_input|test_get_alarm_pixmap' karabogui"
        unset ACCEPT_SIGSEGV
        # TODO: figure out why these tests fail on MacOs
        safeRunCommand "nosetests -v $COVER_FLAGS -e 'test_actual_timestamp|test_floor_divide|test_fmod|test_mod|test_remainder' karabo.native"
        safeRunCommand "nosetests -v $COVER_FLAGS karabo.common"
        conda deactivate
        return 0
    fi
    # Allow gui tests to crash sometimes - for the time being:
    ACCEPT_SIGSEGV=true
    safeRunCommand "nosetests -v $COVER_FLAGS -e test_get_alarm_pixmap karabogui"
    unset ACCEPT_SIGSEGV
    safeRunCommand "nosetests -v $COVER_FLAGS karabo.native"
    safeRunCommand "nosetests -v $COVER_FLAGS karabo.common"

    conda deactivate
}

runPythonUnitTests() {
    echo
    echo Running Karabo Python unit tests ...
    echo

    # Pass the bound_api/launcher.py file. If the file is imported, a
    # part of its code is executed. That results in an error.
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.bound_api"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.bound_devices"
    # Some middlelayer tests are flaky for the time being, so add proper flags:
    safeRunCommand "$NOSETESTS -v $FLAKY_FLAGS $COVER_FLAGS karabo.middlelayer_api"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.middlelayer_devices"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.common"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.macro_api"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.macro_devices"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.influxdb"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.native"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.project_db"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.config_db"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.tests"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.interactive"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.macro_api"
    # Allow gui tests to crash sometimes - for the time being:
    # MR-3871: disable while karabogui doesn't support Qt5 on the old deps
    #ACCEPT_SIGSEGV=true
    #safeRunCommand "$NOSETESTS -v $COVER_FLAGS -e test_extensions_dialog karabogui"
    #unset ACCEPT_SIGSEGV

    echo
    echo Karabo Python unit tests complete
    echo
}


runPythonIntegrationTests() {
    echo
    echo Running Karabo Python integration tests ...
    echo

    # TODO: Needs to be uncommented when the bound_device_test integration test is added.
    #safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.bound_device_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.all_api_alarm_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.device_comm_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.device_provided_scenes_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.pipeline_processing_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.device_cross_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.device_schema_injection_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.config_manager_cross_test"
    # Rerun the middlelayer here for UVLOOP business
    export KARABO_UVLOOP=1
    safeRunCommand "$NOSETESTS -v $FLAKY_FLAGS $COVER_FLAGS karabo.middlelayer_api"
    unset KARABO_UVLOOP
    echo
    echo Karabo Python integration tests complete
    echo
}

runPythonLongTests() {
    echo
    echo Running Karabo Python long tests ...
    echo

    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.pipeline_cross_test"

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
            --runCondaUnitTests)
                RUN_CONDA_UNIT_TEST=true
                ;;
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

if $RUN_CONDA_UNIT_TEST; then
    runCondaUnitTests
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
