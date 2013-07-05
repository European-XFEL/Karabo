#!/bin/bash

CWD=$(pwd)

DEPENDENCIES_Linux=( python2.7 lapack numpy scipy sip pyqt4 pyqwt5 ipython qt4 guiqwt guidata boost openmqc hdf5 snmp log4cpp cppunit openmq)
DEPENDENCIES_Linux_NoGui=( python2.7 ipython boost openmqc hdf5 snmp log4cpp cppunit )
DEPENDENCIES_Darwin=( pythonBundleMacOSX boost openmqc hdf5 snmp log4cpp cppunit )

#################################################################################################

if [ "$(uname -s)" = "Linux" ]; then
    DEPENDENCIES=( ${DEPENDENCIES_Linux[@]} )
elif [ "$(uname -s)" = "Darwin" ]; then
    DEPENDENCIES=( ${DEPENDENCIES_Darwin[@]} )
fi

# $1 EXTERN_DIR      Path to karabo extern folder
# $2 INSTALL_PREFIX  Installation prefix
# $3 WHAT            Specified libraries (optional)
EXTERN_DIR=$1
INSTALL_PREFIX=$2
OPT_GUI=$3
if [ "$(uname -s)" = "Linux" -a $OPT_GUI = "NOGUI" ]; then
    DEPENDENCIES=( ${DEPENDENCIES_Linux_NoGui[@]} )
fi
shift;shift;shift
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
