#!/bin/bash

# Script for creating a full Karabo software bundle with the C++ portions
# of the Framework built with CMake.
#
# This script expects the following positional parameters:
#  1. DISTDIR (e.g. "dist"),
#  2. CONF (e.g. "Debug"),
#  3. BUNDLE_ACTION(package|install),
#  4. PYOPT (normal|develop),
#  5. EXTERN_DEPS_DIR (e.g. "/home/john/karabo-extern-deps")
#
#  EXTERN_DEPS_DIR is the root directory with Karabo Framework's third-
#  party dependencies. Those include C++ dependencies, like boost and gzip,
#  Python dependencies, like the Python runtime itself and tools involved in
#  the builds for Karabo and its devices, like cmake.
#

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
BUNDLE_ACTION=$3
PYOPT=$4
EXTERN_DEPS_DIR=$5
OS=$(uname -s)
MACHINE=$(uname -m)
PACKAGENAME=karabo
VERSION=$(git describe --exact-match --tags HEAD 2>/dev/null)

NUM_CORES=2  # default
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs | sed -r "s/^([0-9]+).*/\1/")
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
fi

scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
BASEDIR=$(get_abs_path $scriptDir/../../)
EXTRACT_SCRIPT=$scriptDir/extract.sh
PYTHON_FIXER_SCRIPT=$scriptDir/fix-python-scripts.sh
PYTHONPATH_FIXER_SCRIPT=$scriptDir/fix-python-path.sh
PACKAGEDIR=$BASEDIR/package/$CONF/$DISTRO_ID/$DISTRO_RELEASE/$MACHINE/$PACKAGENAME
INSTALLSCRIPT=${PACKAGENAME}-${CONF}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

# Carefully clean the bundle
if [ -d $PACKAGEDIR ]; then
    rm -rf $PACKAGEDIR/activate $PACKAGEDIR/extern
fi
rm -f $BASEDIR/karabo

# Start fresh
mkdir -p $PACKAGEDIR
mkdir -p $PACKAGEDIR/plugins
mkdir -p $PACKAGEDIR/var/log
mkdir -p $PACKAGEDIR/var/data

# karabo external dependencies
cp -rf $EXTERN_DEPS_DIR $PACKAGEDIR/extern
if [ "$OS" == "Linux" ]; then
    cp $scriptDir/karaboPackageDependencies-GNU-Linux-x86.pc $PACKAGEDIR/lib/karaboDependencies.pc
fi

# karathon
# Use karabo embedded python interpreter
PATH=$PACKAGEDIR/extern/bin:$PATH
SITE_PACKAGES=`python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"`

cp -rf $PACKAGEDIR/lib/libkarathon.so $SITE_PACKAGES/karathon.so # <-- karathon.so
# ln -s $SITE_PACKAGES/karathon.so $PACKAGEDIR/lib/libkarathon.so
[ -d $PACKAGEDIR/extern/include/python3.8m ]  && (cd $PACKAGEDIR/extern/include; rm -f python3.8; ln -s python3.8m python3.8)

# deviceServer, brokerMessageLogger, idxview, idxbuild, brokerRates are
# packaged by building the install target of their cmake projects.

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
safeRunCommand "$scriptDir/build-pythonKarabo.sh" $PACKAGEDIR $PYOPT

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
cd $scriptDir
cp bundle-cppplugin.sh $PACKAGEDIR/bin/.bundle-cppplugin.sh
cp bundle-pythonplugin.sh $PACKAGEDIR/bin/.bundle-pythonplugin.sh
cp bundle-dependency.sh $PACKAGEDIR/bin/.bundle-dependency.sh
cp bundle-pythondependency.sh $PACKAGEDIR/bin/.bundle-pythondependency.sh
cp extract-cppplugin.sh $PACKAGEDIR/bin/.extract-cppplugin.sh
cp extract-pythonplugin.sh $PACKAGEDIR/bin/.extract-pythonplugin.sh
cp extract-dependency.sh $PACKAGEDIR/bin/.extract-dependency.sh
cp extract-pythondependency.sh $PACKAGEDIR/bin/.extract-pythondependency.sh
cp fix-python-scripts.sh $PACKAGEDIR/bin/.fix-python-scripts.sh
cp fix-python-path.sh $PACKAGEDIR/bin/.fix-python-path.sh
cp set_relative_rpath.py $PACKAGEDIR/bin/.set_relative_rpath.py

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