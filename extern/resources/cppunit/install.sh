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

DEP_DIR=cppunit-1.12.1

if [ ! -d $DEP_DIR ]; then
    echo "Building $DEP_DIR..."
    echo "Unpacking files, please wait..."
    tar -xzf ${DEP_DIR}.tar.gz
fi
  cd $DEP_DIR
  ./configure --prefix=`pwd`/cppunit
  make -j 2>&1 | tee make.log
  make install
  cp -rf cppunit/include ${INSTALL_PREFIX}
  cp -rf cppunit/lib ${INSTALL_PREFIX}

cd $CWD