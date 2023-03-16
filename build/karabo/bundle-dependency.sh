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
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
  exit 1
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
source "$KARABO/bin/.set_lsb_release_info.sh"
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $LSB_RELEASE_DIST )
    DISTRO_RELEASE=$(echo $LSB_RELEASE_VERSION | sed -r "s/^([0-9]+).*/\1/")
fi

DEPVERSION=$(git rev-parse --short HEAD)

DEPNAME=`basename $originalPwd`
PACKAGENAME=$DEPNAME-$DEPVERSION-$KARABOVERSION
EXTRACT_SCRIPT=$KARABO/bin/.extract-dependency.sh
INSTALLSCRIPT=${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh
DISTDIR=$originalPwd/localdist
PACKAGEDIR=$originalPwd/package
SITE_PACKAGES_DIR=`$PYTHON -c "import site; print(site.getsitepackages()[0])"`
LOCAL_SITE_PACKAGES_DIR=$DISTDIR/${SITE_PACKAGES_DIR##*/extern/}

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
