#!/usr/bin/env bash

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

karaboCondaGetScriptPath() {
    # This function allows to follow links and identify the path of this script.
    # since this script should run in OsX as well, we cannot simply use the `-f`
    # option of readlink
    ORIGIN_PWD=$PWD
    # get the absolute path
    # in linux one would not need this trick. This will work also on BSD based systems like OsX
    SCRIPT_PATH=$(cd "$(dirname ${BASH_SOURCE[0]})" ; pwd -P)
    cd $SCRIPT_PATH
    TARGET_FILE=$(basename ${BASH_SOURCE[0]})
    # follow the symlinks if any
    while [ -L "$TARGET_FILE" ]
    do
        TARGET_FILE=`readlink $TARGET_FILE`
        SCRIPT_PATH=$(cd "$(dirname ${TARGET_FILE})" ; pwd -P)
        cd $SCRIPT_PATH
        TARGET_FILE=`basename $TARGET_FILE`
    done
    cd $ORIGIN_PWD
    unset ORIGIN_PWD
}

karaboCondaCleanEnvironment() {
    typeset env_name="$1"
    if [ "${CLEAN}" != true ]; then
        # NOP
        return 0
    fi
    # exit environment that will be wiped, if we are in it.
    if [ "${CONDA_DEFAULT_ENV}" == "${env_name}" ]; then
        safeRunCondaCommand conda deactivate || safeRunCondaCommand source deactivate || return 1
    fi
    # wipe the environment
    if [[ `conda info --envs | grep ${env_name}` != "" ]]; then
        echo "### removed '${env_name}' conda environment ###"
        safeRunCondaCommand conda env remove -n ${env_name} -y || return 1
    else
        echo "### '${env_name}' conda environment inexistent, nothing to clean ###"
    fi
    if [ "$SETUP_FLAG" != false ]; then
        _RECIPE_DIR=${SCRIPT_PATH}/conda-recipes/${KARABO_ENV}
        echo
        echo "### Creating '${env_name}' conda environment ###"
        echo
        # create the base environment programmatically
        safeRunCondaCommand conda activate || safeRunCondaCommand source activate || return 1
        safeRunCondaCommand conda devenv --file ${_RECIPE_DIR}/environment.devenv.yml || return 1
        unset _RECIPE_DIR
    fi
}

karaboCondaCheckBaseEnvironment() {
    echo "> build_conda_env.sh::karaboCondaCheckBaseEnvironment: CONDA_DEFAULT_ENV = $CONDA_DEFAULT_ENV"
    # make sure we are in the default environment
    if [ -z "${CONDA_DEFAULT_ENV}" ]; then
        safeRunCondaCommand conda activate || safeRunCondaCommand source activate || safeRunCondaCommand activate base || return 1
    elif [ "${CONDA_DEFAULT_ENV}" != "base" ]; then
        echo "> build_conda_env.sh::karaboCondaCheckBaseEnvironment: will activate base environment"
        safeRunCondaCommand conda activate || return 1
    fi

    # check if the base environment is sufficient to setup the recipes
    BUILD_PKGS=("conda-build" "conda-devenv" "cogapp" "setuptools_scm")
    for pkg in "${BUILD_PKGS[@]}"; do
        echo "> build_conda_env.sh::karaboCondaCheckBaseEnvironment: checking for pkg = $pkg"
        if [ -z "$(conda list ${pkg} | grep -v '#')" ]; then
            echo "Conda environment missing package from needed packages ${BUILD_PKGS}"
            return 1
        fi
    done
}

karaboCondaInstallGUIEnvironment() {
    KARABO_ENV=karabogui
    # Clean environment if needed
    karaboCondaCleanEnvironment ${KARABO_ENV} || return 1
    if [ "$SETUP_FLAG" == false ]; then
        # NOP
        return 0
    fi

    safeRunCondaCommand conda activate ${KARABO_ENV} || return 1
    # We are breaking the module integrity of pythonKarabo, from which we only need pythonKarabo/common and /native.
    # This flag is used in the setup.py to filter which modules to select. Ideally native and common should
    # be in their own package or at least their own conda recipe.
    pushd ${SCRIPT_PATH}/src/pythonKarabo
    export BUILD_KARABO_SUBMODULE=NATIVE
    safeRunCondaCommand python3 setup.py ${SETUP_FLAG} || return 1
    unset BUILD_KARABO_SUBMODULE
    popd
    pushd ${SCRIPT_PATH}/src/pythonGui
    safeRunCondaCommand python3 setup.py ${SETUP_FLAG} || return 1
    popd
}

displayHelp() {
    echo "
Usage: build_conda_env.sh install|develop|clean [envs]

This script should be sourced so the environment is activated in the caller shell.
If you don't source it, just run 'conda activate karabogui' in the end.
The optional list of environments will allow one to develop/install multiple
conda development environments. No environment means the karabogui environment
for backward compatibility.

Usage example:

    source build_conda_env.sh clean develop

    Will:
        - Clean the 'karabogui' environment
        - Create it again
        - Install 'karabogui' in development mode

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
ENVS=()

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
        karabogui|karabo-cpp|karabo-mdl)
            echo "> build_conda_env.sh: arg[1] = $1"
            if [ "${SETUP_FLAG}" == false ]; then
                displayHelp
                return 1
            fi
            ENVS+=($1)
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


if [ "${#ENVS[@]}" == "0" ]; then
    ENVS+=("karabogui")
fi

if [[ "${SETUP_FLAG}" == false && "${CLEAN}" == false ]]; then
    displayHelp
    return 1
fi


CONDA_ROOT=`conda info --json | grep root_prefix | awk -F ": " '{printf $2;}' | sed  's|[,"]||g'`
CONDA_LOCAL_CHANNEL="file://${CONDA_ROOT}/conda-bld"
CONDA_LOCAL_CHANNEL_ENV="  - ${CONDA_LOCAL_CHANNEL}"
# functions are not exported to subshells.
source ${CONDA_ROOT}/etc/profile.d/conda.sh
echo "> build_conda_env.sh: will check conda base environment"
karaboCondaCheckBaseEnvironment || return 1
karaboCondaGetScriptPath

for ENV_NAME in $ENVS; do
    case "$ENV_NAME" in
        karabogui)
            karaboCondaInstallGUIEnvironment || return 1
            ;;
    esac
done
