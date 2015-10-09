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

if [ -z $KARABO ]; then
    if [ -e $HOME/.karabo/karaboFramework ]; then
        KARABO=$(cat $HOME/.karabo/karaboFramework)
        KARABOVERSION=$(cat $KARABO/VERSION)
    else
      echo "ERROR Could not find karabo Framework. Make sure you have installed karabo Framework."
      exit 1
    fi
else
    KARABOVERSION=$(cat $KARABO/VERSION)
fi

if [ "$OS" == "Darwin" ]; then
  PYTHON=python3.4
  PIP=pip
  WHEEL_INSTALL_FLAGS="--user"
else
  PYTHON=$KARABO/extern/bin/python
  PIP=$KARABO/extern/bin/pip
  WHEEL_INSTALL_FLAGS=
fi

DEPNAME=`basename $originalPwd`
DISTDIR=$originalPwd/package

# Always clean the bundle
rm -rf $DISTDIR
# Start fresh
mkdir -p $DISTDIR

###### dependency custom code #################################
source $originalPwd/build.config

###### packaging and installing of dependency to karabo #######

# Use pip to install dependency in Karabo
echo -e "\n### Installing $DEPNAME"
$PIP install $WHEEL_INSTALL_FLAGS $DISTDIR/*.whl --upgrade
echo -e "\n\n**** Installed $DEPNAME"

cd $originalPwd

exit 0

