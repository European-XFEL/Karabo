#!/bin/bash

DISTDIR=$1
CONF=$2
PLATFORM=$3
PACKAGE_TYPE=$4
PACKAGE_OPTION=$5
OS=$(uname -s)
MACHINE=$(uname -m)
tmp=$(svn info ../../../ | grep URL)
VERSION=${tmp##*/}
if [ "$VERSION" = "trunk" ]; then
    tmp=$(svn info ../../../ | grep Revision)
    VERSION=r${tmp##*: }
fi

if [ "$PACKAGE_TYPE" = "tar" ]; then
    if [ $PACKAGE_OPTION = "NOGUI" ]; then
        PACKAGENAME=karabo-nogui-$VERSION
    else 
        PACKAGENAME=karabo-$VERSION
    fi
elif [ "$PACKAGE_TYPE" = "install" ]; then
    PACKAGENAME=karabo
fi

NUM_CORES=2
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
    if [ "$NUM_CORES" -gt "8" ]; then NUM_CORES=8; fi
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
fi
EXTRACT_SCRIPT=$(pwd)/.extract.sh
PACKAGEDIR=$(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/$PACKAGENAME
#INSTALLSCRIPT=karabo-${VERSION}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh
INSTALLSCRIPT=${PACKAGENAME}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

if [ "$PACKAGE_TYPE" = "tar" ]; then
    if [ -d $(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE ]; then rm -rf $(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE; fi
fi
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
if [ $PACKAGE_OPTION = "NOGUI" ]; then
   echo
elif [ $PACKAGE_OPTION = "GUI" ]; then
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
