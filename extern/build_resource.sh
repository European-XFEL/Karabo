#!/bin/bash

# Script for automated building of external dependencies
#
# Author: <burkhard.heisen@xfel.eu>
#
# This script is typically automatically called during the karabo build.
# There should be no need to call it manually...(also it works as reported)
#

# Help function for checking successful execution of commands
checkReturnCode() {
    ret_code=$?
    if [ $ret_code != 0 ]; then
        if [ -n "$1" ]; then
            # if the output is present, print it in case of error
            cat $1>&1
            # remove the temporary file before exiting
            rm -f $1
            # redirect the filedescriptor 3 to stdout as is tradition
            exec 3>&1
        fi
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        exit $ret_code
    fi
}

safeRunCommand() {
    local cmnd="$*"
    local ret_code

    echo cmnd=$cmnd
    eval $cmnd
    checkReturnCode ""
}

safeRunCommandQuiet() {
    local cmnd="$*"
    local ret_code

    tmp_output=$(mktemp)
    echo cmnd=$cmnd
    # redirect the stream with file descriptor 3 to the temporary file
    exec 3>"$tmp_output"
    # execute the command redirecting the output to 3
    eval $cmnd>&3 2>&3
    # `checkReturnCode` will print the output even in quiet mode.
    checkReturnCode $tmp_output
    # remove the temporary file to clean up
    rm -f $tmp_output
    # redirect the filedescriptor 3 to stdout as is tradition
    exec 3>&1
}

usage() {
    echo
    echo "Usage: $0 RESOURCE_NAME [INSTALL_PREFIX]"
    echo
    echo "RESOURCE_NAME  - The resource to be installed out of those possible:"
    echo
    echo "$1"
    echo
    echo "INSTALL_PREFIX - The install location of the selected resource"
    echo
}

CWD=$(pwd)

# Make sure the script runs in the correct directory
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
cd ${scriptDir}
if [ $? -ne 0 ]; then
    echo " Could not change directory to ${scriptDir}"
    exit 1;
fi

##############################################################################
# Parse command line args (anything starting with '-')
QUIETLY="n"
until [ ${1:0:1} != "-" ]; do
    case $1 in
        --help|-help|-h)
        usage "$0 $(ls resources)"
        exit 0
        ;;
        --quiet|-q)
        QUIETLY="y"
        ;;
        *)
        echo "Unrecognized argument '$1'"
        usage "$0 $(ls resources)"
        exit 1
        ;;
    esac
    shift
done

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
source "$scriptDir/../set_lsb_release_info.sh"
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $LSB_RELEASE_DIST )
    DISTRO_RELEASE=$LSB_RELEASE_VERSION
fi


RESOURCE_PATH=resources/$RESOURCE_NAME
if [ -d $RESOURCE_PATH ]; then
    cd $RESOURCE_PATH
    source build.config
    if [ ! $CUSTOM_BUILD ]; then

        echo -e "\n### Extracting $RESOURCE_NAME"
        if [ "$QUIETLY" = "y" ]; then
            safeRunCommandQuiet "$EXTRACT_COMMAND"
        else
            safeRunCommand "$EXTRACT_COMMAND"
        fi
        cd $DEP_NAME

        echo -e "\n### Configuring $RESOURCE_NAME"
        if [ "$QUIETLY" = "y" ]; then
            safeRunCommandQuiet "$CONFIGURE_COMMAND"
        else
            safeRunCommand "$CONFIGURE_COMMAND"
        fi

        echo -e "\n### Compiling $RESOURCE_NAME"
        if [ "$QUIETLY" = "y" ]; then
            safeRunCommandQuiet "$MAKE_COMMAND"
        else
            safeRunCommand "$MAKE_COMMAND"
        fi
        echo -e "\n### Installing $RESOURCE_NAME"
        if [ "$QUIETLY" = "y" ]; then
            safeRunCommandQuiet "$INSTALL_COMMAND"
        else
            safeRunCommand "$INSTALL_COMMAND"
        fi
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

