#!/bin/bash

CWD=$(pwd)

DEPENDENCIES_Linux=( python3.4 setuptools pip wheel cython lapack numpy scipy libpng freetype nose pillow qt4 sip pyqt4 backports tornado pyparsing six dateutil pytz matplotlib pyqwt5 guidata guiqwt pexpect pyzmq markupsafe jinja2 pygments docutils alabaster babel snowballstemmer sphinx_rtd_theme sphinx ipython boost hdf5 h5py log4cpp cppunit openmq openmqc pyusb parse quamash suds jsonschema ecdsa pycrypto paramiko tzlocal patchelf gmock snappy jpeg httplib2 pssh tiff traits pint )
# NoGui should be obsoleted as qt4 and pyqt4 is needed for matplotlib backed as well as pyqwt5, guiqwt and guidata are used in CLI
DEPENDENCIES_Linux_NoGui=( python3.4 setuptools pip wheel lapack numpy scipy nose libpng freetype pillow matplotlib pexpect tornado pyzmq pygments docutils sphinx ipython boost openmqc hdf5 h5py log4cpp cppunit openmq pyusb parse snappy jpeg httplib2 pssh tiff traits pint )
DEPENDENCIES_Darwin=( setuptools pip wheel pyqwt5 guidata guiqwt boost openmqc hdf5 h5py log4cpp cppunit parse snappy traits pint )

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


MARKER_PATH=$INSTALL_PREFIX/.marker.txt

# Move the marker file from its old location if it's there.
if [ -f ./.marker.txt ]; then mv ./.marker.txt $MARKER_PATH; fi

#check if $MARKER_PATH exists or not => if it does then read contents.
#Otherwise create the file and input names of packages one by one as they are installed.
if [ ! -f $MARKER_PATH ]; then touch $MARKER_PATH; fi

# Read the marker file into a variable as a list
IFS=$'\r\n' MARKER=$(cat $MARKER_PATH)
unset IFS

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
    if [ $vin -eq 0 -a "$WHAT" = "" ]; then
        continue
    fi
    $EXTERN_DIR/install.sh $i $INSTALL_PREFIX    
    rv=$?
    if [ $rv -ne 0 ]; then 
        echo 
        echo "### PROBLEMS building $i, exiting... ###"
        echo
        exit $rv
    fi
    echo $i >> $MARKER_PATH
done

cd $CWD
echo "### All external dependencies successfully installed/already present. ###";
