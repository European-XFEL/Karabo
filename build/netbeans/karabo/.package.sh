#!/bin/bash

DISTDIR=$1
CONF=$2
PLATFORM=$3
OS=$(uname -s)
MACHINE=$(uname -m)
PACKAGEDIR=$(pwd)/../../../package/karabo-$CONF-$OS-$MACHINE
NUM_CORES=2
# Find number of cores on machine
if [ "$(uname -s)" = "Linux" ]; then
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
    NUM_CORES=$(($NUM_CORES*2/3))
fi
echo "### INFO Building is preferentially parallelized into $NUM_CORES threads."

if [ -d $PACKAGEDIR ]; then rm -rf $PACKAGEDIR; fi
mkdir -p $PACKAGEDIR

# karabo
make -j$NUM_CORES CONF=$CONF CMD=y
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/
cp -rf ../../../extern/$PLATFORM $PACKAGEDIR/extern

# deviceServer
cd ../deviceServer
make -j$NUM_CORES CONF=$CONF CMD=y
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# brokerMessageLogger
cd ../brokerMessageLogger
make -j$NUM_CORES CONF=$CONF CMD=y
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# pythonGui
cd ../pythonGui
./build.sh
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/

# pythonCli
cd ../pythonCli
./build.sh
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/

# Tar it
cd $PACKAGEDIR/../
tar -zcf karabo-$CONF-$OS-$MACHINE.tar.gz karabo-$CONF-$OS-$MACHINE

echo 
echo "Successfully created package: $PACKAGEDIR"
echo
