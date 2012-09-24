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

LOG4CPP_DIR=log4cpp-1.0

if [ ! -d $LOG4CPP_DIR ]; then
    echo "Building $LOG4CPP_DIR..."
    echo "Unpacking files, please wait..."
    tar -xzf ${LOG4CPP_DIR}.tar.gz
fi

  cd $LOG4CPP_DIR
  ./configure --prefix=$DIR/$LOG4CPP_DIR/log4cpp 2>&1 | tee configure.log
  
  make -j 2>&1 | tee make.log
  make install
  cp -rf log4cpp/include ${INSTALL_PREFIX}
  cp -rf log4cpp/lib ${INSTALL_PREFIX}
  cp -rf log4cpp/bin ${INSTALL_PREFIX}

cd $CWD