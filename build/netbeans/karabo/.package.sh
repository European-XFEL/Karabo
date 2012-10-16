#!/bin/bash

DISTDIR=$1
CONF=$2
PLATFORM=$3
PACKAGE_TYPE=$4
OS=$(uname -s)
MACHINE=$(uname -m)
tmp=$(svn info ../../../ | grep URL)
VERSION=${tmp##*/}
if [ "$VERSION" = "trunk" ]; then
    tmp=$(svn info ../../../ | grep Revision)
    VERSION=r${tmp##*: }
fi
PACKAGENAME=karabo-$VERSION

if [ "$OS" = "Linux" ]; then
    DISTRO_ID=$(lsb_release -is)
    DISTRO_RELEASE=$(lsb_release -rs)
    tmp=`grep "processor" /proc/cpuinfo | wc -l`
    NUM_CORES=$(($tmp*2/3))
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=2
fi
EXTRACT_SCRIPT=$(pwd)/.extract.sh
PACKAGEDIR=$(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/$PACKAGENAME
INSTALLSCRIPT=karabo-${VERSION}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

if [ -d $(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE ]; then rm -rf $(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE; fi
mkdir -p $PACKAGEDIR

# karabo
make -j$NUM_CORES CONF=$CONF
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/
cp -rf ../../../extern/$PLATFORM $PACKAGEDIR/extern
if [ $OS = "Darwin" ]; then
    cd $PACKAGEDIR/lib
    ln -s libkarabo.dylib libkarabo.so
    cd -
fi

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

if [ "$PACKAGE_TYPE" = "tar" ]; then
    # Tar it
    cd $PACKAGEDIR/../
    tar -zcf ${PACKAGENAME}.tar.gz $PACKAGENAME
    
    # Create installation script
    echo -e '#!/bin/bash\n'"VERSION=$VERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
    chmod +x $INSTALLSCRIPT
elif [ "$PACKAGE_TYPE" = "install" ]; then
    mkdir -p $HOME/.karabo
    echo $PACKAGEDIR > $HOME/.karabo/karaboFramework
fi
echo 
echo "Successfully created package: $PACKAGEDIR"
echo
