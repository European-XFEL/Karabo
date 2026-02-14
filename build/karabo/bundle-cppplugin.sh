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

    eval $cmnd
    ret_code=$?
    if [ $ret_code != 0 ]; then
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        exit $ret_code
    fi
}

DISTDIR=$1
CONF=$2
PLATFORM=$3

OS=$(uname -s)
TARGET_ARCH=$(uname -m)
REPO_TAG=$(git rev-parse --short HEAD)

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
  exit 1
else
    KARABOVERSION=$(cat $KARABO/VERSION)
fi

PLUGINNAME=`basename $(pwd)`
PACKAGENAME=$PLUGINNAME-$REPO_TAG-$KARABOVERSION

NUM_CORES=2  # default
source "$KARABO/bin/.set_lsb_release_info.sh"
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $LSB_RELEASE_DIST )
    DISTRO_RELEASE=$(echo $LSB_RELEASE_VERSION | sed -r "s/^([0-9]+).*/\1/")
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
fi

EXTRACT_SCRIPT=$KARABO/bin/.extract-cppplugin.sh
PACKAGEDIR=$(pwd)/package
INSTALLSCRIPT=${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${TARGET_ARCH}-${CONF}.sh

# Always clean the bundle
rm -rf $PACKAGEDIR

# Start fresh
mkdir -p $PACKAGEDIR

# Copy shared library and includes to PACKAGEDIR
if [ -d $DISTDIR/$CONF/$PLATFORM ]; then
    find $DISTDIR/$CONF/$PLATFORM -maxdepth 1 -type f -name \*.so -exec cp {} $PACKAGEDIR \;
    [ -d $DISTDIR/$CONF/$PLATFORM/include ] && cp -r $DISTDIR/$CONF/$PLATFORM/include $PACKAGEDIR
else
    echo -e "\nUnknown location for project shared library\n"
    exit 1
fi

# run custom script
if [ -e $(pwd)/custom.sh ]; then $(pwd)/custom.sh; fi

cd $PACKAGEDIR
if [ -d include ]; then
    safeRunCommand "tar -zcf ${PACKAGENAME}.tar.gz *.so include"
else
    safeRunCommand "tar -zcf ${PACKAGENAME}.tar.gz *.so"
fi
# Create installation script
echo -e '#!/bin/bash\n'"VERSION=$REPO_TAG\nPLUGINNAME=$PLUGINNAME\nKARABOVERSION=$KARABOVERSION" | cat - $EXTRACT_SCRIPT ${PACKAGENAME}.tar.gz > $INSTALLSCRIPT
chmod a+x $INSTALLSCRIPT

# Clean up
safeRunCommand "rm -rf ${PACKAGENAME}.tar.gz $PACKAGEDIR/*.so"
[ -d $PACKAGEDIR/include ] && safeRunCommand "rm -rf $PACKAGEDIR/include"

echo
echo "Created package: ${PACKAGEDIR%/*}/$INSTALLSCRIPT"
echo
