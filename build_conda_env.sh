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
        return ${ret_code}
    fi
}

displayHelp() {
    echo "
Usage: $0 [install|develop|clean]

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
      "clean" cleans the environment"
}

SETUP_FLAG=false
CLEAN=false

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
            return 1
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

KARABO_ENV=karabogui

SCRIPT_PATH=$(dirname $(readlink -f "$0"))

# Clean environment if asked
if [ "${CLEAN}" = true ]; then
    safeRunCommand conda deactivate || safeRunCommand source deactivate || return 1
    safeRunCommand conda env remove -n ${KARABO_ENV} -y || return 1
fi

echo
echo "### Creating conda environment ###"
echo

safeRunCommand conda devenv --file ${SCRIPT_PATH}/src/pythonGui/environment.devenv.yml || return 1
safeRunCommand conda activate ${KARABO_ENV} || safeRunCommand source activate ${KARABO_ENV} || return 1

if [ "$SETUP_FLAG" != false ]; then
    # We are breaking the module integrity of pythonKarabo, from which we only need pythonKarabo/common and /native.
    # This flag is therefore used in the setup.py to filter which modules to select. Ideally native and common should
    # be in their own package
    export BUILD_KARABO_GUI=1
    pushd ${SCRIPT_PATH}/src/pythonKarabo
    safeRunCommand python setup.py ${SETUP_FLAG} || return 1
    popd
    pushd ${SCRIPT_PATH}/src/pythonGui
    safeRunCommand python setup.py ${SETUP_FLAG} || return 1
    popd
fi
