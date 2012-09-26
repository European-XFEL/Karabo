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

RESOURCE_PATH=$DIR/resources/$RESOURCE_NAME
if [ -d $RESOURCE_PATH ]; then
    $RESOURCE_PATH/install.sh $INSTALL_PREFIX
    rv=$?
else
    echo
    echo "### ERROR  Resource $RESOURCE_NAME does not exist."
    echo
    rv=1
fi

cd $CWD

exit $rv

