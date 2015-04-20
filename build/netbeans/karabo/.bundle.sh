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
    if [ "$DISTRO_ID" = "Scientific" ]; then
       DISTRO_RELEASE=${DISTRO_RELEASE%%\.*}
    fi
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

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
    ln -sf karathon.dylib karathon_bin.so
    #install_name_tool -add_rpath "$PACKAGEDIR/lib" karathon.so
    #install_name_tool -add_rpath "$PACKAGEDIR/extern/lib" karathon.so
    cd -

else
    # Use karabo embedded python interpretor
    PATH=$PACKAGEDIR/extern/bin:$PATH
    PYTHON_INTERPRETOR=`python3 -c "import os,sys;print(os.path.realpath(sys.argv[1]))" $PACKAGEDIR/extern/bin/python3`
    PYKARABO=`python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"`/karabo
    [ -e $PYKARABO ] && [ ! -d $PYKARABO ] && echo "Cannot create $PYKARABO directory"
    [ -d $PYKARABO ] && rm -rf $PYKARABO/* # <-- clean 
    [ ! -d $PYKARABO ] && mkdir $PYKARABO  # <-- create PYKARABO if needed

    # Copying 'extern' has resulted in changing python interpreter path for scripts in 'bin' directory
    # <-- replace 1st line by proper interp path
    [ -f $PACKAGEDIR/extern/bin/ipython ]           && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/ipython
    [ -f $PACKAGEDIR/extern/bin/ipython3 ]          && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/ipython3
    [ -f $PACKAGEDIR/extern/bin/ipython3.4-config ] && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/ipython3.4-config
    [ -f $PACKAGEDIR/extern/bin/2to3 ]              && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/2to3
    [ -f $PACKAGEDIR/extern/bin/cygdb ]             && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/cygdb
    [ -f $PACKAGEDIR/extern/bin/cython ]            && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/cython
    [ -f $PACKAGEDIR/extern/bin/guidata-tests ]     && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/guidata-tests
    [ -f $PACKAGEDIR/extern/bin/guiqwt-tests ]      && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/guiqwt-tests
    [ -f $PACKAGEDIR/extern/bin/idle ]              && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/idle
    [ -f $PACKAGEDIR/extern/bin/ipcluster ]         && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/ipcluster
    [ -f $PACKAGEDIR/extern/bin/ipcontroller ]      && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/ipcontroller
    [ -f $PACKAGEDIR/extern/bin/ipengine ]          && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/ipengine
    [ -f $PACKAGEDIR/extern/bin/iplogger ]          && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/iplogger
    [ -f $PACKAGEDIR/extern/bin/iptest ]            && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/iptest
    [ -f $PACKAGEDIR/extern/bin/irunner ]           && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/irunner
    [ -f $PACKAGEDIR/extern/bin/nosetests ]         && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/nosetests
    [ -f $PACKAGEDIR/extern/bin/nosetests-3.4 ]     && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/nosetests-3.4
    [ -f $PACKAGEDIR/extern/bin/pycolor ]           && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/pycolor
    [ -f $PACKAGEDIR/extern/bin/pydoc ]             && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/pydoc
    [ -f $PACKAGEDIR/extern/bin/sift ]              && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/sift
    [ -f $PACKAGEDIR/extern/bin/smtpd.py ]          && sed -i '1 s%^.*$%#!/usr/bin/env python3%g' $PACKAGEDIR/extern/bin/smtpd.py
    # TODO: Some files from 'bin' are still not converted: pyuic4
fi

cp -rf $DISTDIR/$CONF/$PLATFORM/lib/karathon.so $PYKARABO/karathon_bin.so # <-- karathon.so 
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/

# Check if symbolic link "libkarathon.so" exists. If not, create it! 
if [ ! -L $PYKARABO/libkarathon.so ]; then 
    cd $PYKARABO/                       # go to python 'site-packages/karabo' directory
    ln -s karathon_bin.so libkarathon.so    # create symbolic link
    cd -                                # return back to previous working directory
fi


# deviceServer
cd ../deviceServer
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# brokerMessageLogger
cd ../brokerMessageLogger
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# pythonKarabo
cd ../pythonKarabo
safeRunCommand "./build.sh"
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
cp -rf $DISTDIR/$OS/lib/pythonKarabo/karabo/. $PYKARABO/  	# <-- 'karabo' package: __init__.py, ...                      

# pythonGui
if [ $BUNDLE_OPTION = "NoGui" ]; then
   echo
elif [ $BUNDLE_OPTION = "Gui" ]; then
   cd ../pythonGui
   safeRunCommand "./build.sh"
   cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
   cp -rf $DISTDIR/$OS/lib $PACKAGEDIR/
fi

# pythonCli
cd ../pythonCli
safeRunCommand "./build.sh"
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
cp -rf $DISTDIR/$OS/lib/pythonCli/. $PYKARABO/

# serverControl
cd ../serverControl
safeRunCommand "./build.sh"
cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
cp -rf $DISTDIR/$OS/lib/serverControl/. $PYKARABO/

# pythonTools (deprecated)
#cd ../pythonTools
#safeRunCommand "./build.sh"
#cp -rf $DISTDIR/$OS/bin $PACKAGEDIR/
#cp -rf $DISTDIR/$OS/lib/pythonTools/. $PYKARABO/

# run (Karabo's run/package development environment)
#tmpPwd=$(pwd)
#cp -rf ../../../run $PACKAGEDIR/karaboRun
tar --exclude=.svn -cf - ../../../run 2>/dev/null | ( cd $PACKAGEDIR; tar xf - ; mv run karaboRun)
#cd $PACKAGEDIR
#tar -czf KaraboRun-$VERSION.tar.gz KaraboRun-$VERSION
# Version information
echo $VERSION > $PACKAGEDIR/karaboRun/VERSION
#rm -rf KaraboRun-$VERSION
#cd $tmpPwd


# bundle scripts for plugin packages
cd ../karabo
cp .bundle-cppplugin.sh .bundle-pythonplugin.sh .extract-cppplugin.sh .extract-pythonplugin.sh $PACKAGEDIR/bin
cp .extract-dependency.sh .set_relative_rpath.py .bundle-dependency.sh $PACKAGEDIR/bin

if [ "$OS" = "Linux" ]; then
	PACKAGEDIR=$(readlink -f $PACKAGEDIR)
fi

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
