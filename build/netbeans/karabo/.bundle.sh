#!/bin/bash

# Script for creating a full karabo software bundle
#
# Author: <burkhard.heisen@xfel.eu>
#
# 
# This script is intended to run in conjunction with the NetBeans build system.
# It should be called from within the NetBeans Makefile and expects the following parameters:
#
# DISTDIR (e.g. "dist"), CONF (e.g. "Debug"), PLATFORM (e.g. "GNU-Linux-x86"), BUNDLE_ACTION(package|install), BUNDLE_OPTION(Gui|NoGui)
#

get_abs_path() {
     local PARENT_DIR=$(dirname "$1")
     cd "$PARENT_DIR"
     local ABS_PATH="$(pwd -P)"/"$(basename "$1")"
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
BUNDLE_OPTION=$5
OS=$(uname -s)
MACHINE=$(uname -m)
if tmp=$(svn info ../../../ | grep URL)
then
    VERSION=${tmp##*/}
    if [ "$VERSION" = "trunk" ]; then
        tmp=$(svn info ../../../ | grep Revision)
        VERSION=r${tmp##*: }
    fi
elif tmp=$(jsvn info ../../../ | grep URL)
then
    VERSION=${tmp##*/}
    if [ "$VERSION" = "trunk" ]; then
        tmp=$(jsvn info ../../../ | grep Revision)
        VERSION=r${tmp##*: }
    fi
else
    VERSION=$(git rev-parse --short HEAD)
fi

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
    if [ "$DISTRO_ID" = "Scientific" -o "$DISTRO_ID" = "CentOS" ]; then
       DISTRO_RELEASE=${DISTRO_RELEASE%%\.*}
    fi
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

EXTRACT_SCRIPT=$(pwd)/.extract.sh
PYTHON_FIXER_SCRIPT=$(pwd)/.fix-python-scripts.sh
PACKAGEDIR=$(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/$PACKAGENAME
INSTALLSCRIPT=${PACKAGENAME}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

# Always clean the bundle
rm -rf $PACKAGEDIR

# Start fresh
mkdir -p $PACKAGEDIR

# Normalize the PACKAGEDIR path (This must happen after mkdir!)
PACKAGEDIR=$(get_abs_path $PACKAGEDIR)

# Clean above, if we create a new package
if [[ "$BUNDLE_ACTION" = "package" ]]; then
    if [ -d $(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE ]; then 
        rm -rf $(pwd)/../../../package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE
    fi
fi

# Version information
echo $VERSION > $PACKAGEDIR/VERSION

# karabo
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/
cp -rf ../../../extern/$PLATFORM $PACKAGEDIR/extern
cp karaboPackageDependencies-${PLATFORM}.pc $PACKAGEDIR/lib/karaboDependencies.pc

# karathon
cd ../karathon

if [ "$OS" = "Darwin" ]; then

    # On MacOSX we are currently not bundling the python environment, 
    # but rather use the OS provided one

    # Location of python package: karabo (not within site-packages under MacOSX)
    PYKARABO=$PACKAGEDIR/lib/karabo
    mkdir -p $PYKARABO

    cd $DISTDIR/$CONF/$PLATFORM/lib
    # Python needs to see bindings as .so, even on MacOSX
    ln -sf karathon.dylib karathon.so
    #install_name_tool -add_rpath "$PACKAGEDIR/lib" karathon.so
    #install_name_tool -add_rpath "$PACKAGEDIR/extern/lib" karathon.so
    cp karathon* $PYKARABO/
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
cd ../../../src/tools/scripts/
cp -f * $PACKAGEDIR/bin
cd -

# Correct python interpreter path for scripts in 'bin' directory
# <-- replace 1st line by "/usr/bin/env python3" and set PATH
safeRunCommand "$PYTHON_FIXER_SCRIPT" $(readlink -f $PACKAGEDIR)
export PATH=$PACKAGEDIR/extern/bin:$PATH

# pythonKarabo
cd ../pythonKarabo
safeRunCommand "./build.sh" $PACKAGEDIR
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/

# pythonGui
if [ $BUNDLE_OPTION = "NoGui" ]; then
   echo
elif [ $BUNDLE_OPTION = "Gui" ]; then
   cd ../pythonGui
   safeRunCommand "./build.sh"
   cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
   cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/
fi

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
    pushd ../../../doc
    safeRunCommand "./build.sh" $PACKAGEDIR $VERSION
    cp -rf .build/html $PACKAGEDIR/docs
    popd
fi

# run (Karabo's run/package development environment)
cd ../../../
tar --exclude=.svn --exclude=run/servers/dataLoggerServer/karaboHistory -cf - run 2>/dev/null | ( cd $PACKAGEDIR; tar xf - ; mv run karaboRun)
# Activation script
sed "s%__VENV_DIR__%$PACKAGEDIR%g" run/bin/activate.tmpl > $PACKAGEDIR/karaboRun/bin/activate
# Version information
echo $VERSION > $PACKAGEDIR/karaboRun/VERSION
cd -

# bundle scripts for plugin packages
cd ../karabo
cp .bundle-cppplugin.sh .bundle-pythonplugin.sh $PACKAGEDIR/bin
cp .bundle-dependency.sh .bundle-pythondependency.sh $PACKAGEDIR/bin
cp .extract-cppplugin.sh .extract-pythonplugin.sh $PACKAGEDIR/bin
cp .extract-dependency.sh $PACKAGEDIR/bin
cp .fix-python-scripts.sh .set_relative_rpath.py $PACKAGEDIR/bin

if [ "$OS" = "Linux" ]; then
	PACKAGEDIR=$(readlink -f $PACKAGEDIR)
fi

safeRunCommand "$PACKAGEDIR/bin/.fix-python-scripts.sh $PACKAGEDIR"

if [ "$BUNDLE_ACTION" = "package" ]; then
    # Tar it
    cd $PACKAGEDIR/../
    safeRunCommand "tar -zcf ${PACKAGENAME}.tar.gz $PACKAGENAME"
    
    # Create installation script
    echo -e '#!/bin/bash\n'"VERSION=$VERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
    chmod a+x $INSTALLSCRIPT
    ln -sf $PACKAGEDIR karabo
    PACKAGEDIR=$(pwd)/karabo
fi

# Update the karabo installation location
mkdir -p $HOME/.karabo
echo $PACKAGEDIR > $HOME/.karabo/karaboFramework

echo
echo "Created karaboFramework bundle under: $PACKAGEDIR"
echo
