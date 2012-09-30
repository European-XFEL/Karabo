#!/bin/bash

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
        echo; echo "### Extracting $RESOURCE_NAME..." | tee -a $CWD/installExtern.log
        $EXTRACT_COMMAND
        if [ $? -ne 0 ]; then exit $?; fi
        cd $DEP_NAME
        echo; echo "### Conifguring $RESOURCE_NAME..." | tee -a $CWD/installExtern.log
        echo $CONFIGURE_COMMAND
        eval $CONFIGURE_COMMAND 2>&1 | tee -a configure.log
        if [ $? -ne 0 ]; then exit $?; fi
        echo; echo "### Compiling $RESOURCE_NAME..." | tee -a $CWD/installExtern.log
        eval $MAKE_COMMAND  2>&1 | tee -a $CWD/installExtern.log
        if [ $? -ne 0 ]; then exit $?; fi
        eval $INSTALL_COMMAND
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

