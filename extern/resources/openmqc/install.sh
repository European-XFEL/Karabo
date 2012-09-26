#!/bin/bash

INSTALL_PREFIX=$1
CWD=$(pwd)
DIR=`dirname $0`
cd ${DIR}

###
#
# Any dependency must install headers to:
#
#    $INSTALL_PREFIX/include
#
# and libraries to:
#
#    $INSTALL_PREFIX/lib
#
# Optionally binaries can be installed to:
#
#    $INSTALL_PREFIX/bin
#
# And documentation to:
#
#    $INSTALL_PREFIX/doc
#
###

OPENMQC_DIR=openmqc_4_4_0

echo "Building $OPENMQC_DIR..."
if [ ! -d $OPENMQC_DIR ]; then
    echo "Unpacking files, please wait..."
    tar -xzf ${OPENMQC_DIR}.tar.gz
fi

cd $OPENMQC_DIR
make -j CONF=$(uname -s) CMD=y

cp -rf include/openmqc $INSTALL_PREFIX/include
cp -rf lib $INSTALL_PREFIX
echo "done"

cd $CWD