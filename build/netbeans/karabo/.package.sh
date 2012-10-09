#!/bin/bash

DISTDIR=$1
CONF=$2
PLATFORM=$3
OS=$(uname -s)
MACHINE=$(uname -m)
PACKAGEDIR=$(pwd)/../../../package/karabo-$CONF-$OS-$MACHINE

if [ -d $PACKAGEDIR ]; then rm -rf $PACKAGEDIR; fi
mkdir -p $PACKAGEDIR

# karabo
make -j4 CONF=$CONF CMD=y
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/
cp -rf ../../../extern/$PLATFORM $PACKAGEDIR/extern

# deviceServer
cd ../deviceServer
make -j4 CONF=$CONF CMD=y
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# brokerMessageLogger
cd ../brokerMessageLogger
make -j4 CONF=$CONF CMD=y
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
