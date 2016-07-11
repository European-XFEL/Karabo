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

    echo cmnd=$cmnd
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
VERSION=$(git describe --exact-match HEAD 2>/dev/null)
if [ $? -ne 0 ]; then
    # Otherwise use the short hash
    VERSION=$(git rev-parse --short HEAD)
fi
PACKAGENAME=karabo-$VERSION

NUM_CORES=2  # default
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    if [ "$DISTRO_ID" = "Scientific" -o "$DISTRO_ID" = "CentOS" ]; then
       DISTRO_RELEASE=${DISTRO_RELEASE%%\.*}
    fi
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

BASEDIR=$(get_abs_path $(pwd)/../../../)
EXTRACT_SCRIPT=$(pwd)/.extract.sh
PYTHON_FIXER_SCRIPT=$(pwd)/.fix-python-scripts.sh
PACKAGEDIR=$BASEDIR/package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/$PACKAGENAME
INSTALLSCRIPT=${PACKAGENAME}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

# Always clean the bundle
if [ -d $BASEDIR/package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE ]; then
    rm -rf $BASEDIR/package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE
fi
rm -f $BASEDIR/karabo

# Start fresh
mkdir -p $PACKAGEDIR

# Version information
echo $VERSION > $PACKAGEDIR/VERSION

# karabo
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/
cp -rf $BASEDIR/extern/$PLATFORM $PACKAGEDIR/extern
cp karaboPackageDependencies-${PLATFORM}.pc $PACKAGEDIR/lib/karaboDependencies.pc

# karathon
cd ../karathon

if [ "$OS" = "Darwin" ]; then

    # On MacOSX we are currently not bundling the python environment, 
    # but rather use the OS provided one

    SITE_PACKAGES=`/opt/local/bin/python -c "from site import USER_SITE; print(USER_SITE)"`

    # Location of python package: karabo (not within site-packages under MacOSX)
    # PYKARABO=$PACKAGEDIR/lib/karabo
    # mkdir -p $PYKARABO

    cd $DISTDIR/$CONF/$PLATFORM/lib
    # Python needs to see bindings as .so, even on MacOSX
    ln -sf karathon.dylib karathon.so
    #install_name_tool -add_rpath "$PACKAGEDIR/lib" karathon.so
    #install_name_tool -add_rpath "$PACKAGEDIR/extern/lib" karathon.so
    cp karathon* $SITE_PACKAGES/
    cd -

else
    # Use karabo embedded python interpretor
    PATH=$PACKAGEDIR/extern/bin:$PATH
    SITE_PACKAGES=`python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"`

    cp -rf $DISTDIR/$CONF/$PLATFORM/lib/karathon.so $SITE_PACKAGES/ # <-- karathon.so

    [ -d $PACKAGEDIR/extern/include/python3.4m ]  && (cd $PACKAGEDIR/extern/include; rm -f python3.4; ln -s python3.4m python3.4)
fi

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

#shell scripts - copy directly from src
cd $BASEDIR/src/tools/scripts/
cp -f * $PACKAGEDIR/bin
cd -

# Correct python interpreter path for scripts in 'bin' directory
# <-- replace 1st line by "/usr/bin/env python3" and set PATH
safeRunCommand "$PYTHON_FIXER_SCRIPT" $PACKAGEDIR
export PATH=$PACKAGEDIR/extern/bin:$PATH

# pythonKarabo
cd ../pythonKarabo
safeRunCommand "./build.sh" $PACKAGEDIR $PYOPT
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/

# pythonGui
cd ../pythonGui
safeRunCommand "./build.sh" $PACKAGEDIR $PYOPT
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/

# serverControl
cd ../serverControl
safeRunCommand "./build.sh"
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/

# pythonTools (deprecated)
#cd ../pythonTools
#safeRunCommand "./build.sh"
#cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/

if [ "$BUNDLE_ACTION" = "package" ]; then
    # Build the docs
    pushd $BASEDIR/doc
    safeRunCommand "./build.sh" $PACKAGEDIR $VERSION
    cp -rf .build/html $PACKAGEDIR/docs
    popd
fi

# run (Karabo's run/package development environment)
cd $BASEDIR
tar --exclude=run/servers/karaboHistory -cf - run 2>/dev/null | ( cd $PACKAGEDIR; tar xf - ; mv run karaboRun)
# Activation script
sed "s%__VENV_DIR__%$BASEDIR/karabo%g" src/tools/scripts/activate.tmpl > $PACKAGEDIR/activate
ln -s $PACKAGEDIR/activate $PACKAGEDIR/karaboRun/activate
# Version information
echo $VERSION > $PACKAGEDIR/karaboRun/VERSION
cd -

# bundle scripts for plugin packages
cd ../karabo
cp .bundle-cppplugin.sh .bundle-pythonplugin.sh $PACKAGEDIR/bin
cp .bundle-dependency.sh .bundle-pythondependency.sh $PACKAGEDIR/bin
cp .extract-cppplugin.sh .extract-pythonplugin.sh $PACKAGEDIR/bin
cp .extract-dependency.sh .extract-pythondependency.sh $PACKAGEDIR/bin
cp .fix-python-scripts.sh .set_relative_rpath.py $PACKAGEDIR/bin

safeRunCommand "$PACKAGEDIR/bin/.fix-python-scripts.sh $PACKAGEDIR"

if [ "$BUNDLE_ACTION" = "package" ]; then
    # Tar it
    cd $PACKAGEDIR/../
    safeRunCommand "tar -zcf ${PACKAGENAME}.tar.gz $PACKAGENAME"
    
    # Create installation script
    echo -e '#!/bin/bash\n'"VERSION=$VERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
    chmod a+x $INSTALLSCRIPT
fi

# Make sure the ~/.karabo directory exists
mkdir -p $HOME/.karabo

# Create a shortcut to the installed karabo bundle
cd $BASEDIR
ln -sf $PACKAGEDIR karabo


echo
echo "Created karaboFramework bundle under: $PACKAGEDIR"
echo
