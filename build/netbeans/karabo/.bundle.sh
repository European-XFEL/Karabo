#!/bin/bash

# Script for creating a full karabo software bundle
#
# Author: <burkhard.heisen@xfel.eu>
#
# 
# This script is intended to run in conjunction with the NetBeans build system.
# It should be called from within the NetBeans Makefile and expects the following parameters:
#
# DISTDIR (e.g. "dist"), CONF (e.g. "Debug"), PLATFORM (e.g. "GNU-Linux-x86"), BUNDLE_ACTION(package|install|clean), BUNDLE_OPTION(Gui|NoGui)
#

#### Parameter setup
DISTDIR=$1
CONF=$2
PLATFORM=$3
BUNDLE_ACTION=$4
BUNDLE_OPTION=$5
OS=$(uname -s)
MACHINE=$(uname -m)
tmp=$(svn info ../../../ | grep URL)
VERSION=${tmp##*/}
if [ "$VERSION" = "trunk" ]; then
    tmp=$(svn info ../../../ | grep Revision)
    VERSION=r${tmp##*: }
fi

#### Bundled clean

if [ $BUNDLE_ACTION = "clean" ]; then
    # karabo
    make clobber
    rm -rf $DISTDIR

    # karathon
    cd ../karathon
    make clobber
    rm -rf $DISTDIR

    # deviceServer
    cd ../deviceServer
    make clobber
    rm -rf $DISTDIR

    # brokerMessageLogger
    cd ../brokerMessageLogger
    make clobber
    rm -rf $DISTDIR

    if [ -d $(pwd)/../../../package/$CONF ]; then
        rm -rf $(pwd)/../../../package/$CONF
    fi

    exit 0
fi

##### Installing or packaging

if [ "$BUNDLE_ACTION" = "package" ]; then
    if [ $BUNDLE_OPTION = "NoGui" ]; then
        PACKAGENAME=karabo-nogui-$VERSION
    else 
        PACKAGENAME=karabo-$VERSION
    fi
elif [ "$BUNDLE_ACTION" = "install" ]; then
    PACKAGENAME=karabo
fi

NUM_CORES=2  # default
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

# Cut the total number to ensure memory fitness
if [ "$NUM_CORES" -gt "8" ]; then NUM_CORES=8; fi

EXTRACT_SCRIPT=$(pwd)/.extract.sh
PACKAGEDIR=$(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/$PACKAGENAME
INSTALLSCRIPT=${PACKAGENAME}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

# Always clean the bundle
rm -rf $PACKAGEDIR

# Clean above, if we create a new package
if [[ "$BUNDLE_ACTION" = "package" ]]; then
    if [ -d $(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE ]; then 
        rm -rf $(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE
    fi
fi

# Start fresh
mkdir -p $PACKAGEDIR


#### Building

# karabo
make -j$NUM_CORES CONF=$CONF
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/
cp -rf ../../../extern/$PLATFORM $PACKAGEDIR/extern
if [ $OS = "Darwin" ]; then
    cd $PACKAGEDIR/lib
    # Python needs to see bindings as .so, even on MacOSX
    ln -sf libkarabo.dylib libkarabo.so
    cd -
fi

# karathon
cd ../karathon
make -j$NUM_CORES CONF=$CONF
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/

# deviceServer
cd ../deviceServer
make -j$NUM_CORES CONF=$CONF
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# brokerMessageLogger
cd ../brokerMessageLogger
make -j$NUM_CORES CONF=$CONF
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# pythonKarabo
cd ../pythonKarabo
./build.sh
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/


# pythonGui
if [ $BUNDLE_OPTION = "NoGui" ]; then
   echo
elif [ $BUNDLE_OPTION = "Gui" ]; then
   cd ../pythonGui
   ./build.sh
   cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
   cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/
fi

# pythonCli
cd ../pythonCli
./build.sh
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/

if [ "$OS" = "Linux" ]; then
	PACKAGEDIR=$(readlink -f $PACKAGEDIR)
fi

if [ "$BUNDLE_ACTION" = "package" ]; then
    # Tar it
    cd $PACKAGEDIR/../
    tar -zcf ${PACKAGENAME}.tar.gz $PACKAGENAME
    
    # Create installation script
    echo -e '#!/bin/bash\n'"VERSION=$VERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
    chmod +x $INSTALLSCRIPT
    ln -sf $PACKAGEDIR karabo
    PACKAGEDIR=$(pwd)/karabo
fi

# Update the karabo installation location
mkdir -p $HOME/.karabo
echo $PACKAGEDIR > $HOME/.karabo/karaboFramework

echo
echo "Created karaboFramework bundle under: $PACKAGEDIR"
echo
