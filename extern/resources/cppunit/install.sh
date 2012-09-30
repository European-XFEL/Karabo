#!/bin/bash

INSTALL_PREFIX=$1
CWD=$(pwd)
DIR=`dirname $0`
cd ${DIR}

echo "Building $DEP_DIR..."
echo "Unpacking files, please wait..."
tar -xzf ${DEP_DIR}.tar.gz

  cd $DEP_DIR
  ./configure --prefix=`pwd`/cppunit
  make -j 2>&1 | tee make.log
  make install
  cp -rf cppunit/include ${INSTALL_PREFIX}
  cp -rf cppunit/lib ${INSTALL_PREFIX}

cd $CWD