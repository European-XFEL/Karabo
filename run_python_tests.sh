#!/bin/bash

# WARNING: This script needs to be run in a proper environment.
# The karabo activate file needs to be sourced and a broker needs to be
# started.

# This variables are created to ensure that 'nosetest' and 'coverage'
# are executed from the karabo environment.
COVERAGE="python $(which coverage)"
NOSETESTS="python $(which nosetests)"

# coverage configuration file
COVERAGE_CONF_FILE=".coveragerc"

# A directory to which the code coverage will be stored.
CODE_COVERAGE_DIR="python_code_coverage_reports"

displayHelp()
{
    echo "
Usage: $0 [flags]

Available flags:
  --help, -h - Display help.
  --runUnitTests - Run Python unit tests.
  --runIntegrationTests - Run Python integration tests.
  --collectCoverage - Collect Python code coverage.
  --generateCoverageReport - Collect Python code coverage and generate coverage report.
"
}

# Set-up coverage tool to also collect coverage data in sub-processes.
setupCoverageTool() {
    echo "Entering setupCoverageTool."

    # Set coverage variables.
    COVER_COVERED_PACKAGES=""
    COVER_COVERED_PACKAGES+="--cover-package=karabo "
    COVER_COVERED_PACKAGES+="--cover-package=karabo_gui "
    COVER_COVERED_PACKAGES+="--cover-package=karabogui "
    COVER_COVERED_PACKAGES+="--cover-inclusive"
    COVER_FLAGS="--with-coverage $COVER_COVERED_PACKAGES"

    # Path to the sitecustomize.py file.
    SITE_PACKAGES_DIR=$(pwd)/karabo/extern/lib/python3.4/site-packages
    SITE_CUSTOMIZE_FILE_PATH=$SITE_PACKAGES_DIR/sitecustomize.py

    export SITE_CUSTOMIZE_FILE_CREATED=false

    # Create '.coveragerc' file.
    echo "
[run]
  source = $(pwd)/package/Debug/Ubuntu/16/x86_64/karabo/
  parallel = True
  data_file = $(pwd)/.coverage
" > $COVERAGE_CONF_FILE

    if [ ! -f $SITE_CUSTOMIZE_FILE_PATH ]; then
        # Create the sitecustomize.py script.

        export SITE_CUSTOMIZE_FILE_CREATED=true

        echo "Creating the sitecustomize.py file. path = '$SITE_CUSTOMIZE_FILE_PATH'"

        echo "import coverage" > $SITE_CUSTOMIZE_FILE_PATH
        echo "coverage.process_startup()" >> $SITE_CUSTOMIZE_FILE_PATH
    fi

    # Enable coverage to collect the code coverage data from all the processes
    # in the following tests.
    export COVERAGE_PROCESS_START=$(pwd)/.coveragerc
    # Instruct 'coverage' to display a message when creating a data file.
    export COVERAGE_DEBUG=dataio
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
        onExit
        exit $ret_code
    fi
}

runPythonUnitTests() {
    echo
    echo Running Karabo Python unit tests ...
    echo

    # WARNING: The authenticator tests are excluded because services used by
    # the tests are not available. Remove the exclusion flags when the services
    # are available.

    # Pass the bound_api/launcher.py file. If the file is imported, a
    # part of its code is executed. That results in an error.
    safeRunCommand \
        "$NOSETESTS -v $COVER_FLAGS " \
            "-I launcher.py " \
            "-e test_authenticator_correct_login " \
            "-e test_authenticator_incorrect_login " \
            "-e test_authenticator_single_sign_on " \
        "karabo.bound_api"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.middlelayer_api"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.common"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.project_db"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.usermacro_api"
    safeRunCommand \
        "$NOSETESTS -v $COVER_FLAGS " \
            "-e test_py_authenticator_incorrect_login " \
            "-e test_py_authenticator_correct_login " \
        "karabo.tests"
    # We will not focus on the older version of GUI.
    # safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo_gui"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabogui"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.interactive"

    echo
    echo Karabo Python unit tests complete
    echo
}

runPythonIntegrationTests() {
    echo
    echo Running Karabo Python integration tests ...
    echo 

    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.bound_device_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.device_comm_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.device_provided_scenes_test"
    safeRunCommand "$NOSETESTS -v $COVER_FLAGS karabo.integration_tests.run_configuration_group"

    echo
    echo Karabo Python integration tests complete
    echo
}

generateCodeCoverageReport() {
    # What to omit when generating html coverage reports reports.
    OMIT="*/site-packages/karabo/*tests*","*/site-packages/karabo/*_test.py"

    echo
    echo Generating HTML coverage reports ...
    echo

    # Combine all the data files.
    coverage combine

    # Remove code coverage directory if it exists.
    rm -rf $CODE_COVERAGE_DIR

    # Generate coverage for the following modules.
    safeRunCommand coverage html -i --include "*/site-packages/karabo/*" --omit $OMIT -d "$CODE_COVERAGE_DIR/htmlcov_karabo"
    safeRunCommand coverage html -i --include "*/site-packages/karabo/bound_api/*" --omit $OMIT -d "$CODE_COVERAGE_DIR/htmlcov_bound_api"
    safeRunCommand coverage html -i --include "*/site-packages/karabo/middlelayer_api/*" --omit $OMIT -d "$CODE_COVERAGE_DIR/htmlcov_middlelayer_api"
    safeRunCommand coverage html -i --include "*/site-packages/karabo/common/*" --omit $OMIT -d "$CODE_COVERAGE_DIR/htmlcov_common"
    safeRunCommand coverage html -i --include "*/site-packages/karabo/project_db/*" --omit $OMIT -d "$CODE_COVERAGE_DIR/htmlcov_project_db"
    safeRunCommand coverage html -i --include "*/site-packages/karabo/interactive/*" --omit $OMIT -d "$CODE_COVERAGE_DIR/htmlcov_interactive"
    safeRunCommand coverage html -i --include "*/site-packages/karabo/usermacro_api/*" --omit $OMIT -d "$CODE_COVERAGE_DIR/htmlcov_usermacro_api"
    # We will not focus on the older version of GUI.
    safeRunCommand coverage html -i --include "*/karabogui/*" --omit $OMIT -d htmlcov_karabogui
    # safeRunCommand coverage html -i --include "*/karabo_gui/*" $OMIT -d htmlcov_karabo_gui # older version
    safeRunCommand coverage html -i --include "*/site-packages/karabo/bound_devices/*" --omit $OMIT -d htmlcov_bound_devices

    echo
    echo HTML coverage reports generation complete
    echo

    echo
    echo "### The Python coverage report can be found at $(pwd)/$CODE_COVERAGE_DIR/htmlcov_karabo/index.html."
    echo
}

# Clean the code coverage report and coverage related files.
clean() {
    echo
    echo Cleaning the code coverage report and coverage related files
    echo

    # Clean the previous coverage date.
    coverage erase

    # Remove coverage configuration file file.
    rm -rf $COVERAGE_CONF_FILE

    # Remove code coverage directory if it exists.
    rm -rf $CODE_COVERAGE_DIR
}

# Main

# Parse arguments.

RUN_UNIT_TEST=false
RUN_INTEGRATION_TEST=false
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
            --collectCoverage)
                COLLECT_COVERAGE=true
                ;;
            --generateCoverageReport)
                GENERATE_COVERAGE_REPORT=true
                ;;
            --clean)
                CLEAN=true
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

