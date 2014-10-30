#!/bin/bash

safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    echo cmnd=$cmnd
    eval $cmnd
    ret_code=$?
    if [ $ret_code != 0 ]; then
	printf "Error : [%d] when executing command: '$cmnd'" $ret_code
    echo
	exit $ret_code
    fi
}

originalPwd=$(pwd)

# check if arguments are passed or not
if [ $# -eq 0 ]; then
    echo "No arguments passed from .install.sh, building will proceed as usual.."
else
    echo "Arguments found; assigning arguments to variables and proceeding with build.."
    CND_DIST=$1
    CONF=$2
    PLATFORM=$3
fi

# Make sure the script runs in the correct directory
#scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
#cd ${scriptDir}
#if [ $? -ne 0 ]; then
#    echo " Could not change directory to ${scriptDir}"
#    exit 1;
#fi

if [ -z $KARABO ]; then
    if [ -e $HOME/.karabo/karaboFramework ]; then
        KARABO=$(cat $HOME/.karabo/karaboFramework)
        KARABOVERSION=$(cat $KARABO/VERSION)
    else
      echo "ERROR Could not find karabo Framework. Make sure you have installed karabo Framework."
      exit 1
    fi
else
    KARABOVERSION=$(cat $KARABO/VERSION)
fi

INSTALL_PREFIX=$KARABO/extern
PYTHON=$KARABO/extern/bin/python

if [ -z $INSTALL_PREFIX ]; then
    echo "### WARNING  No install-prefix given (second argument), using default location: $scriptDir"
    INSTALL_PREFIX=$scriptDir
fi

NUM_CORES=2
# Find number of cores on machine
if [ "$(uname -s)" = "Linux" ]; then
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
fi

echo
echo "### INFO Building is parallelized into $NUM_CORES threads."
echo

MACHINE=$(uname -m)
OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    if [ "$DISTRO_ID" = "Scientific" ]; then
       DISTRO_RELEASE=${DISTRO_RELEASE%%\.*}
    fi
fi
# temporary solution
if tmp=$(svn info . | grep URL)
then
    tmp=${tmp%%-*}
    DEPVERSION=${tmp##*/}
    if [ "$DEPVERSION" = "trunk" ]; then
        tmp=$(svn info . | grep Revision)
        DEPVERSION=r${tmp##*: }
    fi
elif tmp=$(jsvn info . | grep URL)
then
    tmp=${tmp%%-*}
    DEPVERSION=${tmp##*/}
    if [ "$DEPVERSION" = "trunk" ]; then
        tmp=$(jsvn info . | grep Revision)
        DEPVERSION=r${tmp##*: }
    fi
else
    DEPVERSION=$(git rev-parse --short HEAD)
fi

DEPNAME=`basename $originalPwd`
PACKAGENAME=$DEPNAME-$DEPVERSION-$KARABOVERSION
EXTRACT_SCRIPT=$KARABO/bin/.extract-dependency.sh
INSTALLSCRIPT=${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh
DISTDIR=$originalPwd/localdist
PACKAGEDIR=$originalPwd/package

# Always clean the bundle
rm -rf $DISTDIR
rm -rf $PACKAGEDIR
# Start fresh
mkdir -p $DISTDIR/lib
mkdir -p $PACKAGEDIR

###################### dependency custom code ######################
source $originalPwd/build.config

##################### packaging and installing of dependency to karabo ######################

# Packaging
cd ${PACKAGEDIR}
safeRunCommand "tar -zcf ${PACKAGENAME}.tar.gz -C ${DISTDIR} ."

# Create installation script
echo -e '#!/bin/bash\n'"VERSION=$DEPVERSION\nDEPNAME=$DEPNAME" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
chmod a+x $INSTALLSCRIPT
echo
echo "Created package: ${PACKAGEDIR}/$INSTALLSCRIPT"
echo

# Use installation script to install dependency in Karabo
echo -e "\n### Installing $DEPNAME in $INSTALL_PREFIX"
./$INSTALLSCRIPT --prefix=$INSTALL_PREFIX
echo -e "\n\n**** Installed $DEPNAME to $INSTALL_PREFIX"

cd $originalPwd

exit 0

