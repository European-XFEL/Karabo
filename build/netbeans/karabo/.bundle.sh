#!/bin/bash

# Script for creating a full karabo software bundle
#
# Author: <burkhard.heisen@xfel.eu>
#
# 
# This script is intended to run in conjunction with the NetBeans build system.
# It should be called from within the NetBeans Makefile and expects the following parameters:
#
# DISTDIR (e.g. "dist"), CONF (e.g. "Debug"), PLATFORM (e.g. "GNU-Linux-x86"), BUNDLE_ACTION(package|install|clean), BUNDLE_OPTION(Gui|NoGui)
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
tmp=$(svn info ../../../ | grep URL)
VERSION=${tmp##*/}
if [ "$VERSION" = "trunk" ]; then
    tmp=$(svn info ../../../ | grep Revision)
    VERSION=r${tmp##*: }
fi

#### Bundled clean

if [ $BUNDLE_ACTION = "clean" ]; then
    # karabo
    make clobber
    rm -rf $DISTDIR
    rm -rf nbproject/Makefile*

    # karathon
    cd ../karathon
    make clobber
    rm -rf $DISTDIR
    rm -rf nbproject/Makefile*

    # deviceServer
    cd ../deviceServer
    make clobber
    rm -rf $DISTDIR
    rm -rf nbproject/Makefile*

    # brokerMessageLogger
    cd ../brokerMessageLogger
    make clobber
    rm -rf $DISTDIR
    rm -rf nbproject/Makefile*

    if [ -d $(pwd)/../../../package/$CONF ]; then
        rm -rf $(pwd)/../../../package/$CONF
    fi

    exit 0
fi

##### Installing or packaging

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
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

# Cut the total number to ensure memory fitness
if [ "$NUM_CORES" -gt "8" ]; then NUM_CORES=8; fi

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


#### Building

# karabo
safeRunCommand "make -j$NUM_CORES CONF=$CONF"
cp -rf $DISTDIR/$CONF/$PLATFORM/lib $PACKAGEDIR/
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/
cp -rf ../../../extern/$PLATFORM $PACKAGEDIR/extern
if [ $OS = "Darwin" ]; then
    cd $PACKAGEDIR/lib
    # Python needs to see bindings as .so, even on MacOSX
    ln -sf libkarabo.dylib libkarabo.so
    cd -
fi

# Use karabo embedded python interpretor
PYTHON_INTERPRETOR=$PACKAGEDIR/extern/bin/python

# Copying 'extern' has resulted in changing python interpreter path for scripts in 'bin' directory
 
[ -f $PACKAGEDIR/extern/bin/ipython ]           && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/ipython  # <-- replace 1st line by proper interpreter path
[ -f $PACKAGEDIR/extern/bin/ipython2.7-config ] && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/ipython2.7-config
[ -f $PACKAGEDIR/extern/bin/2to3 ]              && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/2to3
[ -f $PACKAGEDIR/extern/bin/cygdb ]             && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/cygdb
[ -f $PACKAGEDIR/extern/bin/cython ]            && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/cython
[ -f $PACKAGEDIR/extern/bin/guidata-tests ]     && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/guidata-tests
[ -f $PACKAGEDIR/extern/bin/guiqwt-tests ]      && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/guiqwt-tests
[ -f $PACKAGEDIR/extern/bin/idle ]              && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/idle
[ -f $PACKAGEDIR/extern/bin/ipcluster ]         && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/ipcluster
[ -f $PACKAGEDIR/extern/bin/ipcontroller ]      && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/ipcontroller
[ -f $PACKAGEDIR/extern/bin/ipengine ]          && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/ipengine
[ -f $PACKAGEDIR/extern/bin/iplogger ]          && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/iplogger
[ -f $PACKAGEDIR/extern/bin/iptest ]            && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/iptest
[ -f $PACKAGEDIR/extern/bin/ipython.py ]        && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/ipython.py
[ -f $PACKAGEDIR/extern/bin/irunner ]           && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/irunner
[ -f $PACKAGEDIR/extern/bin/pycolor ]           && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/pycolor
[ -f $PACKAGEDIR/extern/bin/pydoc ]             && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/pydoc
[ -f $PACKAGEDIR/extern/bin/sift ]              && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/sift
[ -f $PACKAGEDIR/extern/bin/smtpd.py ]          && sed -i '1 s%^.*$%#!'${PYTHON_INTERPRETOR}'%g' $PACKAGEDIR/extern/bin/smtpd.py

# TODO: Some files from 'bin' are still not converted: pyuic4


# karathon python extension module (karathon.so) and pythonKarabo (pure python package)
# should go to python site-packages folder, namely, site-packages/karabo
cat << EOF > sitepackages.py
import sys,re
p=re.compile(".*/site-packages")
for path in sys.path:
    m=p.search(path)
    if m:
        print m.group()
        break
EOF
PYKARABO=`${PYTHON_INTERPRETOR} sitepackages.py`/karabo
rm sitepackages.py
#echo "PYKARABO = $PYKARABO"    
[ -e $PYKARABO ] && [ ! -d $PYKARABO ] && echo "Cannot create $PYKARABO directory"
[ -d $PYKARABO ] && rm -rf $PYKARABO/*				# <-- clean 
[ ! -d $PYKARABO ] && mkdir $PYKARABO            		# <-- create PYKARABO if needed

# karathon
cd ../karathon
safeRunCommand "make -j$NUM_CORES CONF=$CONF"
cp -rf $DISTDIR/$CONF/$PLATFORM/lib/. $PYKARABO/        	# <-- karathon.so 
cp -rf $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR/

# deviceServer
cd ../deviceServer
safeRunCommand "make -j$NUM_CORES CONF=$CONF"
cp -rf $DISTDIR/$CONF/$PLATFORM/bin $PACKAGEDIR/

# brokerMessageLogger
safeRunCommand "cd ../brokerMessageLogger"
make -j$NUM_CORES CONF=$CONF
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

if [ "$OS" = "Linux" ]; then
	PACKAGEDIR=$(readlink -f $PACKAGEDIR)
fi

if [ "$BUNDLE_ACTION" = "package" ]; then
    # Tar it
    cd $PACKAGEDIR/../
    safeRunCommand "tar -zcf ${PACKAGENAME}.tar.gz $PACKAGENAME"
    
    # Create installation script
    echo -e '#!/bin/bash\n'"VERSION=$VERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
    chmod +x $INSTALLSCRIPT
    ln -sf $PACKAGEDIR karabo
    PACKAGEDIR=$(pwd)/karabo
fi

# Update the karabo installation location
mkdir -p $HOME/.karabo
echo $PACKAGEDIR > $HOME/.karabo/karaboFramework

echo
echo "Created karaboFramework bundle under: $PACKAGEDIR"
echo
