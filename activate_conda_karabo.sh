#!/usr/bin/env bash

# Script for activating the KaraboGUI conda environment, also offers some
# auxiliary functionality like building the conda package or cleaning the
# environment.

# Help function for checking successful execution of commands
safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    echo cmnd=${cmnd}
    eval ${cmnd}
    ret_code=$?
    if [ ${ret_code} != 0 ]; then
        printf "Error : [%d] when executing command: '${cmnd}'" ${ret_code}
        echo
        echo
        exit ${ret_code}
    fi
}

displayHelp() {
    echo "
Usage: $0 [Develop|Clean] [flags]

This script should be sourced so the environment is activated in the caller shell.

Usage example:

    source activate_conda_karabo.sh Clean Develop --bundle

    Will clean any existing environment, activate it again, install karabogui in
    development mode and also create the conda karabo package.

Available flags:
    --bundle    - Create the karabogui software bundle. Default: no bundle is created!

Note: "Develop" also installs the karabogui in development mode
      "Clean" cleans the environment"
}

DEVELOP=false
CLEAN=false
BUNDLE=false

# Parse command line
while [ -n "$1" ]; do
    case "$1" in
        Develop)
            DEVELOP=true
            shift
            ;;
        Clean)
            CLEAN=true
            shift
            ;;
        --bundle)
            BUNDLE=true
            shift
            ;;
        -h|--help)
            displayHelp
            exit 1
            ;;
        *)
            # Make a little noise
            echo "Unrecognized commandline flag: $1"
            exit 1
    esac
done

KARABOENV=karabogui

SCRIPTPATH=$(dirname $(readlink -f "$0"))
echo ${SCRIPTPATH}

if [ "$CLEAN" = true ]; then
    safeRunCommand conda deactivate
    safeRunCommand conda remove -n ${KARABOENV} --all -y
    exit 0
fi

# First let's activate the environment
pushd ${SCRIPTPATH}
cd ./src/pythonGui/

if [ "${BUNDLE}" = true ]; then
    bash ./scripts/build_conda_recipe.sh
    exit 0
fi

echo
echo "### Creating conda environment ###"
echo

safeRunCommand conda devenv
safeRunCommand source activate ${KARABOENV}

if [ "$DEVELOP" = true ]; then
    export KARABO_GUI_DEVELOP=1
    python setup.py develop
fi

popd
