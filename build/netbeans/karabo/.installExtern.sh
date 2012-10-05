#!/bin/bash

CWD=$(pwd)

DEPENDENCIES_Linux=( python2.7 sip pyqt4 pyqwt5 ipython boost openmqc hdf5 snmp log4cpp cppunit )
DEPENDENCIES_Darwin=( boost openmqc hdf5 snmp log4cpp cppunit )

#################################################################################################

DEPENDENCIES=$(eval echo $`echo DEPENDENCIES_$(uname -s)`)

# $1 EXTERN_DIR      Path to karabo extern folder
# $2 INSTALL_PREFIX  Installation prefix
# $3 WHAT            Specified libraries (optional)
EXTERN_DIR=$1
INSTALL_PREFIX=$2
shift;shift
WHAT=$@

if [ "$WHAT" != "" ]; then
    DEPENDENCIES=( $WHAT )
fi

for i in "${DEPENDENCIES[@]}"
do
    :
    $EXTERN_DIR/install.sh $i $INSTALL_PREFIX
    rv=$?
    if [ $rv -ne 0 ]; then 
        echo 
        echo "### PROBLEMS building $i, exiting... ###"
        echo
        exit $rv
    fi
done

cd $CWD
