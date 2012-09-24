#!/bin/bash

INSTALL_PREFIX=$1
CWD=$(pwd)

dir=`dirname $0`
cd ${dir}

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

BOOST_DIR=boost_1_51_0
BOOST_LIBRARIES=date_time,filesystem,regex,thread,system,signals,python

# Find number of cores on machine
if [ "$(uname -s)" = "Linux" ]; then
    num_cores=`grep "processor" /proc/cpuinfo | wc -l`
else
    num_cores=4
fi

if [ ! -d $BOOST_DIR ]; then
    echo "Building $BOOST_DIR..."
    echo "Unpacking files, please wait..."
    tar -xzf ${BOOST_DIR}.tar.gz
fi
    cd $BOOST_DIR
    if [ ! -f bjam ]; then
        ./bootstrap.sh --with-libraries=$BOOST_LIBRARIES  2>&1 | tee configure.log
        ./bjam -j $num_cores 2>&1 | tee make.log
    fi
    cp -rf boost $INSTALL_PREFIX/include
    cp -rf stage/lib $INSTALL_PREFIX
    echo "done"


cd $CWD