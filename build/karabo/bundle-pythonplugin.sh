#!/bin/bash
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

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
TARGET_ARCH=$(uname -m)
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
source "$KARABO/bin/.set_lsb_release_info.sh"
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $LSB_RELEASE_DIST )
    DISTRO_RELEASE=$(echo $LSB_RELEASE_VERSION | sed -r "s/^([0-9]+).*/\1/")
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
fi

EXTRACT_SCRIPT=$KARABO/bin/.extract-pythonplugin.sh
INSTALLSCRIPT=package/${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${TARGET_ARCH}.sh
PYTHON=$KARABO/extern/bin/python3
PIP=$KARABO/extern/bin/pip

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
