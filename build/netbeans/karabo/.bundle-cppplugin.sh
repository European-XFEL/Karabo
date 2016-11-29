#!/bin/bash

safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    #echo cmnd=$cmnd
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
VERSION=$(git rev-parse --short HEAD)

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
  exit 1
else 
    KARABOVERSION=$(cat $KARABO/VERSION)
fi

PLUGINNAME=`basename $(pwd)`
PACKAGENAME=$PLUGINNAME-$VERSION-$KARABOVERSION

NUM_CORES=2  # default
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs | sed -r "s/^([0-9]+).*/\1/")
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

EXTRACT_SCRIPT=$KARABO/bin/.extract-cppplugin.sh
PACKAGEDIR=$(pwd)/package
INSTALLSCRIPT=${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}-${CONF}.sh

# Always clean the bundle
rm -rf $PACKAGEDIR

# Start fresh
mkdir -p $PACKAGEDIR

find $DISTDIR/$CONF/$PLATFORM -maxdepth 1 -type f -name \*.so -exec cp {} $PACKAGEDIR \;

# run custom script
if [ -e $(pwd)/custom.sh ]; then $(pwd)/custom.sh; fi

cd $PACKAGEDIR
safeRunCommand "tar -zcf ${PACKAGENAME}.tar.gz *.so"

# Create installation script
echo -e '#!/bin/bash\n'"VERSION=$VERSION\nPLUGINNAME=$PLUGINNAME\nKARABOVERSION=$KARABOVERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
chmod a+x $INSTALLSCRIPT

# Clean up
safeRunCommand "rm -f ${PACKAGENAME}.tar.gz *.so"

echo
echo "Created package: ${PACKAGEDIR%/*}/$INSTALLSCRIPT"
echo


