#!/bin/bash

originalPwd=$(pwd)
OS=$(uname -s)
MACHINE=$(uname -m)

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
  exit 1
else
    KARABOVERSION=$(cat $KARABO/VERSION)
fi

DEPNAME=`basename $originalPwd`
DISTDIR=$originalPwd/package
PACKAGENAME=$DEPNAME-$KARABOVERSION

source "$KARABO/../../set_lsb_release_info.sh"
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $LSB_RELEASE_DIST )
    DISTRO_RELEASE=$(echo $LSB_RELEASE_VERSION | sed -r "s/^([0-9]+).*/\1/")
fi

EXTRACT_SCRIPT=$KARABO/bin/.extract-pythondependency.sh
INSTALLSCRIPT=$DISTDIR/${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh
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
echo -e '#!/bin/bash\n'"VERSION=$VERSION\nDEPNAME=$DEPNAME\nKARABOVERSION=$KARABOVERSION\nWHEELNAME=$WHEELNAME" | cat - $EXTRACT_SCRIPT $WHEELFILE > $INSTALLSCRIPT
chmod a+x $INSTALLSCRIPT

cd $originalPwd

exit 0

