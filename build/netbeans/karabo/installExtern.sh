#!/bin/bash

DEPENDENCIES=( boost openmqc hdf5 snmp log4cpp cppunit )

# $1 EXTERN_DIR      Path to karabo extern folder
# $2 INSTALL_PREFIX  Installation prefix
EXTERN_DIR=$1
INSTALL_PREFIX=$2

for i in "${DEPENDENCIES[@]}"
do
    :
    $EXTERN_DIR/install.sh $i $INSTALL_PREFIX
    rv=$?
    if [ $rv -ne 0 ]; then 
        echo " ### PROBLEMS building $i, exiting... ###"
        exit $rv
    fi
done
