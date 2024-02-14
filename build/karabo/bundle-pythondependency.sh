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

originalPwd=$(pwd)
OS=$(uname -s)
TARGET_ARCH=$(uname -m)

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
  exit 1
else
    KARABOVERSION=$(cat $KARABO/VERSION)
fi

DEPNAME=`basename $originalPwd`
DISTDIR=$originalPwd/package
PACKAGENAME=$DEPNAME-$KARABOVERSION

source "$KARABO/bin/.set_lsb_release_info.sh"
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $LSB_RELEASE_DIST )
    DISTRO_RELEASE=$(echo $LSB_RELEASE_VERSION | sed -r "s/^([0-9]+).*/\1/")
fi

EXTRACT_SCRIPT=$KARABO/bin/.extract-pythondependency.sh
INSTALLSCRIPT=$DISTDIR/${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${TARGET_ARCH}.sh
PYTHON=$KARABO/extern/bin/python
PIP=$KARABO/extern/bin/pip
WHEEL_INSTALL_FLAGS=


# Always clean the bundle
rm -rf $DISTDIR
# Start fresh
mkdir -p $DISTDIR

###### Run dependency custom code to build the wheel ##########################
source $originalPwd/build.config
retVal=$?
if [ $retVal -ne 0 ]; then
    echo "ERROR: build.config returned $retval. Exiting."
    exit $retVal
fi

# Install the wheel which was created

WHEELNAME=$(basename $DISTDIR/*.whl)
WHEELFILE=$DISTDIR/$WHEELNAME
if [ -f $WHEELFILE ]; then
    $PIP --disable-pip-version-check install -U --no-index $WHEEL_INSTALL_FLAGS $WHEELFILE
    retVal=$?
    if [ $retVal -ne 0 ]; then
        echo "ERROR: pip returned $retval. Exiting."
        exit $retVal
    fi
else
    echo "installation missing wheel in package folder"
    exit 0
fi

# Create a self-extracting installation script
REPO_TAG=$(git rev-parse --short HEAD)
echo -e '#!/bin/bash\n'"VERSION=$REPO_TAG\nDEPNAME=$DEPNAME\nKARABOVERSION=$KARABOVERSION\nWHEELNAME=$WHEELNAME" | cat - $EXTRACT_SCRIPT $WHEELFILE > $INSTALLSCRIPT
chmod a+x $INSTALLSCRIPT

cd $originalPwd

exit 0
