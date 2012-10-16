#!/bin/bash

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
DIR=$(dirname $0)
cd $DIR

# $1 RESOURCE_NAME   -> Name of the installed dependency
# $2 INSTALL_PREFIX  -> Installation prefix
RESOURCE_NAME=$1
INSTALL_PREFIX=$2

if [ -z $INSTALL_PREFIX ]; then
    echo "### WARNING  No install-prefix given (second argument), using default location: $DIR"
    INSTALL_PREFIX=$DIR
fi

mkdir -p $INSTALL_PREFIX/include
mkdir -p $INSTALL_PREFIX/lib
mkdir -p $INSTALL_PREFIX/bin

NUM_CORES=2
# Find number of cores on machine
if [ "$(uname -s)" = "Linux" ]; then
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
    NUM_CORES=$(($NUM_CORES*2/3))
fi

echo
echo "### INFO Building is preferentially parallelized into $NUM_CORES threads."
echo

RESOURCE_PATH=resources/$RESOURCE_NAME
if [ -d $RESOURCE_PATH ]; then
    cd $RESOURCE_PATH
    source build.config
    if [ ! $CUSTOM_BUILD ]; then

        echo -e "\n### Extracting $RESOURCE_NAME" | tee -a $CWD/installExtern.log
        safeRunCommand "$EXTRACT_COMMAND"
        cd $DEP_NAME

        echo -e "\n### Configuring $RESOURCE_NAME" | tee -a $CWD/installExtern.log
        echo $CONFIGURE_COMMAND
        safeRunCommand "$CONFIGURE_COMMAND 2>&1 | tee -a configure.log"

        echo -e "\n### Compiling $RESOURCE_NAME" | tee -a $CWD/installExtern.log
        safeRunCommand "$MAKE_COMMAND  2>&1 | tee -a $CWD/installExtern.log"

	echo -e "\n### Installing $RESOURCE_NAME" | tee -a $CWD/installExtern.log
        safeRunCommand "$INSTALL_COMMAND 2>&1 | tee -a $CWD/installExtern.log"
    fi
    cd $DIR
else
    echo
    echo "### ERROR  Resource $RESOURCE_NAME does not exist."
    echo
    exit 1
fi

cd $CWD

exit 0

