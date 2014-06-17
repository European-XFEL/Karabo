#!/bin/bash

safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    echo cmnd=$cmnd
    eval $cmnd
    ret_code=$?
    if [ $ret_code != 0 ]; then
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        exit $ret_code
    fi
}

DISTDIR=$1
CONF=$2
PLATFORM=$3

OS=$(uname -s)
MACHINE=$(uname -m)
tmp=$(svn info . | grep URL)
VERSION=${tmp##*/}
if [ "$VERSION" = "trunk" ]; then
    tmp=$(svn info . | grep Revision)
    VERSION=r${tmp##*: }
fi

if [ -z $KARABO ]; then
    if [ -e $HOME/.karabo/karaboFramework ]; then
        KARABO=$(cat $HOME/.karabo/karaboFramework)
        KARABOVERSION=$(cat $KARABO/VERSION)
    else
      echo "ERROR Could not find karaboFramework. Make sure you have installed the karaboFramework."
      exit 1
    fi
else 
    KARABOVERSION=$(cat $KARABO/VERSION)
fi

PLUGINNAME=`basename $(pwd)`
PACKAGENAME=$PLUGINNAME-$VERSION-$KARABOVERSION
echo PACKAGENAME $PACKAGENAME

NUM_CORES=2  # default
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    if [ "$DISTRO_ID" = "Scientific" ]; then
       DISTRO_RELEASE=${DISTRO_RELEASE%%\.*}
    fi
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

EXTRACT_SCRIPT=$KARABO/bin/.extract-cppplugin.sh
PACKAGEDIR=$(pwd)/package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/$PACKAGENAME
INSTALLSCRIPT=${PACKAGENAME}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

# Always clean the bundle
rm -rf $PACKAGEDIR

# Start fresh
mkdir -p $PACKAGEDIR

# cp -rf $DISTDIR/$CONF/$PLATFORM/*.so $PACKAGEDIR
find $DISTDIR/$CONF/$PLATFORM -maxdepth 1 -type f -name \*.so -exec cp {} $PACKAGEDIR \;

# run custom script
if [ -e $(pwd)/custom.sh ]; then $(pwd)/custom.sh; fi

cd $PACKAGEDIR/../
safeRunCommand "tar -zcf ${PACKAGENAME}.tar.gz $PACKAGENAME"

# Create installation script
echo -e '#!/bin/bash\n'"VERSION=$VERSION\nPLUGINNAME=$PLUGINNAME\nKARABOVERSION=$KARABOVERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
chmod a+x $INSTALLSCRIPT


echo
echo "Created package: ${PACKAGEDIR%/*}/$INSTALLSCRIPT"
echo


