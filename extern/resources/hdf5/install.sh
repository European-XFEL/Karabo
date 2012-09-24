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

HDF5_DIR=hdf5-1.8.7

if [ ! -d $HDF5_DIR ]; then
    echo "Building $HDF5_DIR..."
    echo "Unpacking files, please wait..."
    tar -xzf ${HDF5_DIR}.tar.gz
fi
  cd $HDF5_DIR
  ./configure --prefix=$DIR/$HDF5_DIR/hdf5 --enable-cxx --enable-production --with-zlib 2>&1 | tee configure.log
  
  make -j 2>&1 | tee make.log
  make install
  cp -rf hdf5/include ${INSTALL_PREFIX}/include/hdf5
  cp -rf hdf5/lib ${INSTALL_PREFIX}
  cp -rf hdf5/bin ${INSTALL_PREFIX}

cd $CWD