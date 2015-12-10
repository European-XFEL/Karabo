#!/bin/bash

safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    echo cmnd=$cmnd
    eval $cmnd
    ret_code=$?
    if [ $ret_code != 0 ]; then
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        echo
        exit $ret_code
    fi
}

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

if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    if [ "$DISTRO_ID" = "Scientific" -o "$DISTRO_ID" = "CentOS" ]; then
       DISTRO_RELEASE=${DISTRO_RELEASE%%\.*}
    fi
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
fi

EXTRACT_SCRIPT=$KARABO/bin/.extract-pythondependency.sh
INSTALLSCRIPT=${PACKAGENAME}-${DISTRO_ID}-${DISTRO_RELEASE}-${MACHINE}.sh

if [ "$OS" == "Darwin" ]; then
  PYTHON=python3.4
  PIP=pip
  WHEEL_INSTALL_FLAGS="--user"
else
  PYTHON=$KARABO/extern/bin/python
  PIP=$KARABO/extern/bin/pip
  WHEEL_INSTALL_FLAGS=
  export PATH=$KARABO/extern/bin/:$PATH
fi


# Always clean the bundle
rm -rf $DISTDIR
# Start fresh
mkdir -p $DISTDIR

###### dependency custom code #################################
source $originalPwd/build.config

# Create installation script
WHEELNAME=$(basename $DISTDIR/*.whl)

echo -e '#!/bin/bash\n'"VERSION=$VERSION\nDEPNAME=$DEPNAME\nKARABOVERSION=$KARABOVERSION\nWHEELNAME=$WHEELNAME" | cat - $EXTRACT_SCRIPT $DISTDIR/$WHEELNAME > $INSTALLSCRIPT
chmod a+x $INSTALLSCRIPT

cd $originalPwd

exit 0

