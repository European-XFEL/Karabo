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

developmentMode=0
stopDevelopment=0

if [ "$1" = "develop" ]; then
    developmentMode=1
    if [ "$2" = "-u" ]; then
        stopDevelopment=1
    fi
fi

OS=$(uname -s)
MACHINE=$(uname -m)
if tmp=$(svn info . | grep URL)
then
    tmp=${tmp%%-*}
    VERSION=${tmp##*/}
    if [ "$VERSION" = "trunk" ]; then
        tmp=$(svn info . | grep Revision)
        VERSION=r${tmp##*: }
    fi
elif tmp=$(jsvn info . | grep URL)
then
    tmp=${tmp%%-*}
    VERSION=${tmp##*/}
    if [ "$VERSION" = "trunk" ]; then
        tmp=$(jsvn info . | grep Revision)
        VERSION=r${tmp##*: }
    fi
else
    VERSION=$(git rev-parse --short HEAD)
fi

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
  exit 1
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
    if [ "$DISTRO_ID" = "Scientific" -o "$DISTRO_ID" = "CentOS" ]; then
       DISTRO_RELEASE=${DISTRO_RELEASE%%\.*}
    fi
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

EXTRACT_SCRIPT=$KARABO/bin/.extract-pythonplugin.sh
INSTALLSCRIPT=package/${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

if [ $OS == "Darwin" ]; then
    PYTHON=/opt/local/bin/python3
    PIP=/opt/local/bin/pip
else
    PYTHON=$KARABO/extern/bin/python3
    PIP=$KARABO/extern/bin/pip
fi

# Always clean the build artifacts
rm -rf build/ dist/ package/

# Create the output directory
mkdir package

if [[ "$developmentMode" == "1" ]]; then
    # Install in the current user's home directory
    if [[ "$stopDevelopment" = "1" ]]; then
        echo
        echo "Uninstalling development package."
        echo
        command="uninstall -y $PLUGINNAME"
    else
        echo
        echo "Installing in development mode. Changes to the source will be immediately available without rebuilding."
        echo
        command="install --user -e ."
    fi
    $PIP --disable-pip-version-check $command
    # We're done
    exit 0
else
    safeRunCommand "$PYTHON setup.py bdist_wheel"
    WHEELNAME=$(basename dist/*.whl)
fi

# run custom script
if [ -e $(pwd)/custom.sh ]; then $(pwd)/custom.sh; fi

# Create installation script
echo -e '#!/bin/bash\n'"VERSION=$VERSION\nPLUGINNAME=$PLUGINNAME\nKARABOVERSION=$KARABOVERSION\nWHEELNAME=$WHEELNAME" | cat - $EXTRACT_SCRIPT dist/$WHEELNAME > $INSTALLSCRIPT
chmod a+x $INSTALLSCRIPT


echo
echo "Created package: $INSTALLSCRIPT"
echo


