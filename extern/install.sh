#!/bin/bash

CWD=$(pwd)

RESOURCE_NAME=$1
RESOURCE_PATH=$CWD/resources/$RESOURCE_NAME
INSTALL_PREFIX=$CWD

if [ -d $RESOURCE_PATH ]; then

    cd $RESOURCE_PATH
    ./install.sh $INSTALL_PREFIX

else
    echo
    echo "Resource $RESOURCE_NAME does not exist."
    echo
fi



