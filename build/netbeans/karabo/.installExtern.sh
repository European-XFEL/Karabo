#!/bin/bash

CWD=$(pwd)

DEPENDENCIES_Linux=( python3.4 cython lapack numpy scipy libpng freetype setuptools nose pillow qt4 sip pyqt4 backports tornado pyparsing six dateutil pytz matplotlib pyqwt5 guidata guiqwt pexpect pyzmq markupsafe jinja2 pygments docutils sphinx ipython boost hdf5 h5py log4cpp cppunit openmq openmqc pyusb parse quamash suds jsonschema ecdsa pycrypto paramiko tzlocal patchelf gmock snappy )
# NoGui should be obsoleted as qt4 and pyqt4 is needed for matplotlib backed as well as pyqwt5, guiqwt and guidata are used in CLI
DEPENDENCIES_Linux_NoGui=( python3.4 lapack numpy scipy nose libpng freetype pillow matplotlib pexpect tornado pyzmq pygments docutils sphinx ipython boost openmqc hdf5 h5py log4cpp cppunit openmq pyusb parse snappy )
DEPENDENCIES_Darwin=( guidata guiqwt boost openmqc hdf5 h5py log4cpp cppunit parse )

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

#save original IFS value
oldIFS=$IFS;

IFS=$'\r\n' MARKER=$(cat ./.marker.txt)

#revert to original IFS value
IFS=$oldIFS;

#function to check existence of element in array
elementIn() {
	local e
    for e in ${@:2}; do	[[ "$e" == "$1" ]] && return 0; done
    return 1
}

#install packages one by one.
for i in "${DEPENDENCIES[@]}"
do
    :
    elementIn "$i" "${MARKER[@]}"
    vin=$?
    if [ $vin -eq 0 -a "$WHAT" = "" ]
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
echo "### All external dependencies successfully installed/already present. ###";
echo "### Now building Karabo... ###";
