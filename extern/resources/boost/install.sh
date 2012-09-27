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
    
    ./bootstrap.sh --prefix=boost_1_51_0 --libdir=boost_1_51_0/lib --with-libraries=$BOOST_LIBRARIES  2>&1 | tee configure.log

    echo "using python : 2.7 : $INSTALL_PREFIX ;" >> project-config.jam
    
    ./b2 -a variant=release -j$num_cores install 2>&1 | tee make.log

    cp -rf boost_1_51_0/include/ $INSTALL_PREFIX
    cp -rf boost_1_51_0/lib/ $INSTALL_PREFIX
    echo "done"


cd $CWD