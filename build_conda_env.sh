#!/usr/bin/env bash

# Script for activating the KaraboGUI conda environment, also offers some
# auxiliary functionality like building the conda package or cleaning the
# environment.

# Help function for checking successful execution of commands
safeRunCondaCommand() {
    typeset cmnd="$*"
    typeset ret_code

    echo cmnd=${cmnd}
    eval ${cmnd}
    ret_code=$?
    if [ ${ret_code} != 0 ]; then
        printf "Error : [%d] when executing command: '${cmnd}'" ${ret_code}
        echo
        echo
        return ${ret_code}
    fi
}

displayHelp() {
    echo "
Usage: $0 [clean] [install|develop]

This script should be sourced so the environment is activated in the caller shell.
If you don't source it, just run 'conda activate karabogui' in the end.

Usage example:

    source build_conda_env.sh clean develop

    Will:
        - Clean any environment
        - Create it again
        - Install karabogui in development mode

    The installation is usually not needed as all code is added in the PYTHONPATH,
    but installing it you will have access to the entrypoints (karabo-gui, etc)

Note: "install" installs karabogui in release mode
      "develop" installs karabogui in development mode
      "clean" cleans the environment

Note: The environment variable XFEL_CONDA_CHANNEL can optionally be used to point
      the build process to alternative Conda repos.
      This is useful, e.g. if one needs to tunnel into a private network.
      If one follows the instructions in doc/installation/gui.rst, this script
      should be called as:
      
      XFEL_CONDA_CHANNEL=localhost:8081 source build_conda_env.sh clean develop"
}

SETUP_FLAG=false
CLEAN=false
KARABO_ENV=karabogui
KARABO_BUILD_ENV=karabo-build

# Parse command line
while [ -n "$1" ]; do
    case "$1" in
        install|develop)
            if [ "${SETUP_FLAG}" != false ]; then
                echo "Can't use 'install' and 'develop' flags at the same time"
                return 1
            fi
            SETUP_FLAG=$1
            shift
            ;;
        clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            displayHelp
            return 0
            ;;
        *)
            # Make a little noise
            echo "Unrecognized commandline flag: $1"
            return 1
    esac
done

if [ "${SETUP_FLAG}" == false ]; then
    displayHelp
    return 1
fi

# functions are not exported to subshells.
source ${CONDA_PREFIX}/etc/profile.d/conda.sh

# make sure we are in the default environment
if [ -z "${CONDA_DEFAULT_ENV}" ]; then
    safeRunCondaCommand conda activate || safeRunCondaCommand source activate || safeRunCondaCommand activate base || return 1
elif [ "${CONDA_DEFAULT_ENV}" != "base" ]; then
    safeRunCondaCommand conda activate || return 1
fi

# Clean environment if asked
if [ ${CLEAN} == true ]; then
    if [ "${CONDA_DEFAULT_ENV}" == "${KARABO_ENV}" ] ; then
        safeRunCondaCommand conda deactivate || return 1
    fi
    safeRunCondaCommand conda env remove -n ${KARABO_ENV} -y || return 1
fi

# check if the base environment is sufficient to setup the recipes
BUILD_PKGS="conda-build conda-devenv cogapp setuptools_scm"
for pkg in $BUILD_PKGS; do
    if [ -z "$(conda list ${pkg} | grep -v '#')" ]; then
        echo "Conda environment missing package from needed packages ${BUILD_PKGS}"
        return 1
    fi
done

echo
echo "### Creating conda environment ###"
echo

ORIGIN_PWD=$PWD
SCRIPT_PATH=$(dirname ${BASH_SOURCE[0]})
cd $SCRIPT_PATH
TARGET_FILE=$(basename ${BASH_SOURCE[0]})
# follow the symlinks if any
while [ -L "$TARGET_FILE" ]
do
    TARGET_FILE=`readlink $TARGET_FILE`
    SCRIPT_PATH=$(dirname $TARGET_FILE)
    cd $SCRIPT_PATH
    TARGET_FILE=`basename $TARGET_FILE`
done
cd $ORIGIN_PWD
unset ORIGIN_PWD

# create the dev environment
safeRunCondaCommand conda devenv --file ${SCRIPT_PATH}/src/pythonGui/environment.devenv.yml || return 1
safeRunCondaCommand conda activate ${KARABO_ENV} || return 1

if [ "$SETUP_FLAG" != false ]; then
    # We are breaking the module integrity of pythonKarabo, from which we only need pythonKarabo/common and /native.
    # This flag is therefore used in the setup.py to filter which modules to select. Ideally native and common should
    # be in their own package
    export BUILD_KARABO_GUI=1
    pushd ${SCRIPT_PATH}/src/pythonKarabo
    safeRunCondaCommand python3 setup.py ${SETUP_FLAG} || return 1
    popd
    pushd ${SCRIPT_PATH}/src/pythonGui
    safeRunCondaCommand python3 setup.py ${SETUP_FLAG} || return 1
    popd
    unset BUILD_KARABO_GUI
fi
