#!/bin/bash

# Script for automated building of external dependencies
#
# Author: <burkhard.heisen@xfel.eu>
#
# This script is typically automatically called during the karabo build.
# There should be no need to call it manually...(also it works as reported)
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
	exit $ret_code
    fi
}

CWD=$(pwd)

# Make sure the script runs in the correct directory
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
cd ${scriptDir}
if [ $? -ne 0 ]; then
    echo " Could not change directory to ${scriptDir}"
    exit 1;
fi

# Parse command line
if [[ -z "$1" ||  $1 = "help" || $1 = "-h" ||  $1 = "-help" || $1 = "--help" ]]; then
    AVAILABLE=$(ls resources)
    cat <<End-of-help
Usage: $0 RESOURCE_NAME [INSTALL_PREFIX]

RESOURCE_NAME  - The resource to be installed out of those possible:

$AVAILABLE

INSTALL_PREFIX - The install location of the selected resource

End-of-help

    exit 0
fi

# $1 RESOURCE_NAME   -> Name of the installed dependency
# $2 INSTALL_PREFIX  -> Installation prefix
RESOURCE_NAME=$1
INSTALL_PREFIX=$2

if [ -z $INSTALL_PREFIX ]; then
    echo "### WARNING  No install-prefix given (second argument), using default location: $scriptDir"
    INSTALL_PREFIX=$scriptDir
fi

mkdir -p $INSTALL_PREFIX/include
mkdir -p $INSTALL_PREFIX/lib
mkdir -p $INSTALL_PREFIX/bin

NUM_CORES=2
# Find number of cores on machine
if [ "$(uname -s)" = "Linux" ]; then
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
fi

echo
echo "### INFO Building is preferentially parallelized into $NUM_CORES threads."
echo

OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
fi


RESOURCE_PATH=resources/$RESOURCE_NAME
if [ -d $RESOURCE_PATH ]; then
    cd $RESOURCE_PATH
    source build.config
    if [ ! $CUSTOM_BUILD ]; then

        echo -e "\n### Extracting $RESOURCE_NAME"
        safeRunCommand "$EXTRACT_COMMAND"
        if [ -d "$DEP_NAME" ]; then
            # Not every package extracts something...
            cd $DEP_NAME
        fi

        echo -e "\n### Configuring $RESOURCE_NAME"
        safeRunCommand "$CONFIGURE_COMMAND"

        echo -e "\n### Compiling $RESOURCE_NAME"
        safeRunCommand "$MAKE_COMMAND"

        echo -e "\n### Installing $RESOURCE_NAME"
        safeRunCommand "$INSTALL_COMMAND"
    fi
    cd $scriptDir
else
    echo
    echo "### ERROR  Resource $RESOURCE_NAME does not exist."
    echo
    exit 1
fi

cd $CWD

exit 0

