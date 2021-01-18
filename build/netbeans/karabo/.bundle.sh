#!/bin/bash

# Script for creating a full karabo software bundle
#
# Author: <burkhard.heisen@xfel.eu>
#
#
# This script is intended to run in conjunction with the NetBeans build system.
# It should be called from within the NetBeans Makefile and expects the following parameters:
#
# DISTDIR (e.g. "dist"), CONF (e.g. "Debug"), PLATFORM (e.g. "GNU-Linux-x86"), BUNDLE_ACTION(package|install)
#

echo "### Now building Karabo... ###";

get_abs_path() {
    local PARENT_DIR=$(dirname "$1")
    local BASENAME=$(basename "$1")
    case $BASENAME in
    ..)
        cd "$1"
        local ABS_PATH="$(pwd -P)"
        ;;
    *)
        cd "$PARENT_DIR"
        local ABS_PATH="$(pwd -P)"/"$BASENAME"
    esac
    cd - >/dev/null
    echo "$ABS_PATH"
}

safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    eval $cmnd
    ret_code=$?
    if [ $ret_code != 0 ]; then
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        exit $ret_code
    fi
}

#### Parameter setup
DISTDIR=$1
CONF=$2
PLATFORM=$3
BUNDLE_ACTION=$4
PYOPT=$5
OS=$(uname -s)
MACHINE=$(uname -m)


# Get the version as a tag name, if one exists
VERSION=$(git describe --exact-match --tags HEAD 2>/dev/null)
if [ $? -ne 0 ]; then
    # Otherwise use the short hash
    VERSION=$(git rev-parse --short HEAD)
fi
PACKAGENAME=karabo

NUM_CORES=2  # default
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs | sed -r "s/^([0-9]+).*/\1/")
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
fi

BASEDIR=$(get_abs_path $(pwd)/../../../)
EXTRACT_SCRIPT=$(pwd)/.extract.sh
PYTHON_FIXER_SCRIPT=$(pwd)/.fix-python-scripts.sh
PYTHONPATH_FIXER_SCRIPT=$(pwd)/.fix-python-path.sh
PACKAGEDIR=$BASEDIR/package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/$PACKAGENAME
INSTALLSCRIPT=${PACKAGENAME}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

# Carefully clean the bundle
if [ -d $PACKAGEDIR ]; then
    rm -rf $PACKAGEDIR/activate $PACKAGEDIR/extern $PACKAGEDIR/include $PACKAGEDIR/lib $PACKAGEDIR/bin
fi
rm -f $BASEDIR/karabo

# Start fresh
mkdir -p $PACKAGEDIR
mkdir -p $PACKAGEDIR/plugins
mkdir -p $PACKAGEDIR/var/log
mkdir -p $PACKAGEDIR/var/data

# Version information
echo $VERSION > $PACKAGEDIR/VERSION

# karabo
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/
cp -rf $BASEDIR/extern/$PLATFORM $PACKAGEDIR/extern
cp karaboPackageDependencies-${PLATFORM}.pc $PACKAGEDIR/lib/karaboDependencies.pc

# karathon
cd ../karathon

# Use karabo embedded python interpretor
PATH=$PACKAGEDIR/extern/bin:$PATH
SITE_PACKAGES=`python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"`

cp -rf $DISTDIR/$CONF/$PLATFORM/lib/karathon.so $SITE_PACKAGES/ # <-- karathon.so
ln -s $SITE_PACKAGES/karathon.so $PACKAGEDIR/lib/libkarathon.so

[ -d $PACKAGEDIR/extern/include/python3.8m ]  && (cd $PACKAGEDIR/extern/include; rm -f python3.8; ln -s python3.8m python3.8)

cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/

# deviceServer
cd ../deviceServer
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# brokerMessageLogger
cd ../brokerMessageLogger
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# idxview
cd ../tools/dataLoggerIndex/idxview
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/
cd ../..

#idxbuild
cd ../tools/dataLoggerIndex/idxbuild
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/
cd ../..

# brokerRates
cd ../tools/brokerRates
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/
cd ../

# shell scripts - copy directly from src
cd $BASEDIR/src/tools/scripts/
cp -f * $PACKAGEDIR/bin
cd -

# Correct python interpreter path for scripts in 'bin' directory
# <-- replace 1st line by "/usr/bin/env python3" and set PATH
safeRunCommand "$PYTHON_FIXER_SCRIPT" $PACKAGEDIR
export PATH=$PACKAGEDIR/extern/bin:$PATH

# Correct Python path in the python config files
safeRunCommand "$PYTHONPATH_FIXER_SCRIPT" $PACKAGEDIR

# pythonKarabo
cd ../pythonKarabo
safeRunCommand "./build.sh" $PACKAGEDIR $PYOPT
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/

# pythonGui
# MR-3871: disable while karabogui doesn't support Qt5 on the old deps
#cd ../pythonGui
#safeRunCommand "./build.sh" $PACKAGEDIR $PYOPT
#cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
#cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/

# Activation script
cd $BASEDIR
sed "s%__VENV_DIR__%$BASEDIR/karabo%g" src/tools/scripts/activate.tmpl > $PACKAGEDIR/activate
# templates
cp -rf src/templates $PACKAGEDIR
# the initial configurations
cp -rf src/service.in $PACKAGEDIR
cp -rf src/environment.in $PACKAGEDIR
cd -

# bundle scripts for plugin packages
cd ../karabo
cp .bundle-cppplugin.sh .bundle-pythonplugin.sh $PACKAGEDIR/bin
cp .bundle-dependency.sh .bundle-pythondependency.sh $PACKAGEDIR/bin
cp .extract-cppplugin.sh .extract-pythonplugin.sh $PACKAGEDIR/bin
cp .extract-dependency.sh .extract-pythondependency.sh $PACKAGEDIR/bin
cp .fix-python-scripts.sh .set_relative_rpath.py $PACKAGEDIR/bin
cp .fix-python-path.sh $PACKAGEDIR/bin



safeRunCommand "$PACKAGEDIR/bin/.fix-python-scripts.sh $PACKAGEDIR"
safeRunCommand "$PACKAGEDIR/bin/.fix-python-path.sh $PACKAGEDIR"

if [ "$BUNDLE_ACTION" = "package" ]; then
    # ZIP it - but exclude developer's stuff from var, devices, etc.
    cd $PACKAGEDIR/../
    safeRunCommand "zip -qr ${PACKAGENAME}.zip $PACKAGENAME --exclude $PACKAGENAME/var/\* $PACKAGENAME/devices/\* $PACKAGENAME/installed/\* $PACKAGENAME/plugins/\*"
    # Create installation script
    echo -e '#!/bin/bash\n'"VERSION=$VERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.zip > $INSTALLSCRIPT
    safeRunCommand "zip -A ${INSTALLSCRIPT}"
    chmod a+x $INSTALLSCRIPT
    rm ${PACKAGENAME}.zip
fi

# Make sure the ~/.karabo directory exists
mkdir -p $HOME/.karabo

# Create a shortcut to the installed karabo bundle
cd $BASEDIR
ln -sf $PACKAGEDIR karabo


echo
echo "Created karaboFramework bundle under: $PACKAGEDIR"
if [ "$BUNDLE_ACTION" = "package" ]; then
    echo "... and installation script: $PACKAGEDIR/../$INSTALLSCRIPT"
fi
echo
