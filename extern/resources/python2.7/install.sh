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
#
###

DEP_DIR=Python-2.7.3

if [ ! -d $DEP_DIR ]; then
    echo "Building $DEP_DIR..."
    echo "Unpacking files, please wait..."
    tar -xzf ${DEP_DIR}.tgz
fi
  cd $DEP_DIR
  
# THIS IS NEEDED FOR USING THE --enabled-shared OPTION IN A NON-STANDARD LOCATION !!!
  mkdir -p `pwd`/python2.7/lib
  mkdir -p `pwd`/python2.7/include
  mkdir -p `pwd`/python2.7/bin

  ./configure --prefix=`pwd`/python2.7 --enable-unicode=ucs4 --enable-shared LDFLAGS="-Wl,-rpath $INSTALL_PREFIX/lib"
  make -j 2>&1 | tee make.log
  make -j install
  cp -rf python2.7/include ${INSTALL_PREFIX}
  cp -rf python2.7/lib ${INSTALL_PREFIX}
  cp -rf python2.7/bin ${INSTALL_PREFIX}

cd $CWD