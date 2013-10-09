#!/bin/bash

CWD=$(pwd)

DEPENDENCIES_Linux=( python2.7 lapack numpy scipy nose libpng freetype matplotlib sip pyqt4 pyqwt5 pexpect tornado pyzmq pygments ipython qt4 guiqwt guidata boost openmqc hdf5 h5py log4cpp cppunit openmq )
DEPENDENCIES_Linux_NoGui=( python2.7 lapack numpy scipy nose libpng freetype matplotlib pexpect tornado pyzmq pygments ipython boost openmqc hdf5 h5py log4cpp cppunit openmq )
DEPENDENCIES_Darwin=( boost openmqc hdf5 log4cpp cppunit )

#################################################################################################

if [ "$(uname -s)" = "Linux" ]; then
    DEPENDENCIES=( ${DEPENDENCIES_Linux[@]} )
elif [ "$(uname -s)" = "Darwin" ]; then
    DEPENDENCIES=( ${DEPENDENCIES_Darwin[@]} )
fi

# $1 EXTERN_DIR      Path to karabo extern folder
# $2 INSTALL_PREFIX  Installation prefix
# $3 OPT_GUI	     With or without GUI
# $4 WHAT            Specified libraries (optional)
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

#check if .marker.txt exists or not => if it does then read contents.
#Otherwise create the file and input names of packages one by one as they are installed.
if [ ! -f ./.marker.txt ]; then touch ./.marker.txt; fi

IFS=$'\r\n' MARKER=($(cat ./.marker.txt))

#function to check existence of element in array
elementIn() {
    local e
    for e in "${@:2}"; do [[ "$e" == "$1" ]] && return 0; done
    return 1
}

#install packages one by one.
for i in "${DEPENDENCIES[@]}"
do
    :
    elementIn "$i" "${MARKER[@]}"
    vin=$?
    if [ $vin -ne 1 ]
    then
	continue;
    fi
    $EXTERN_DIR/install.sh $i $INSTALL_PREFIX    
    rv=$?
    if [ $rv -ne 0 ]; then 
        echo 
        echo "### PROBLEMS building $i, exiting... ###"
        echo
        exit $rv
    fi
    echo $i >> ./.marker.txt
done

cd $CWD
echo "### All Extern dependencies successfully installed/already present. ###";
echo "### Now building karabo... ###";
