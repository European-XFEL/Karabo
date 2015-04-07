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

# Make sure the script runs in the correct directory
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
cd ${scriptDir}
if [ $? -ne 0 ]; then
    echo " Could not change directory to ${scriptDir}"
    exit 1;
fi

if [ -z $KARABO ]; then
    if [ -e $HOME/.karabo/karaboFramework ]; then
        KARABO=$(cat $HOME/.karabo/karaboFramework)
        KARABOVERSION=$(cat $KARABO/VERSION)
    else
      echo "ERROR Could not find karaboFramework. Make sure you have installed the karaboFramework."
      exit 1
    fi
else
    KARABOVERSION=$(cat $KARABO/VERSION)
fi

INSTALL_PREFIX=$KARABO/extern
PYTHON=$KARABO/extern/bin/python

if [ -z $INSTALL_PREFIX ]; then
    echo "### WARNING  No install-prefix given (second argument), using default location: $scriptDir"
    INSTALL_PREFIX=$scriptDir
fi

NUM_CORES=2
# Find number of cores on machine
if [ "$(uname -s)" = "Linux" ]; then
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
fi

echo
echo "### INFO Building is parallelized into $NUM_CORES threads."
echo

MACHINE=$(uname -m)
OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    if [ "$DISTRO_ID" = "Scientific" ]; then
       DISTRO_RELEASE=${DISTRO_RELEASE%%\.*}
    fi
fi

source $originalPwd/build.config
if [ ! $CUSTOM_BUILD ]; then
    
    echo -e "\n### Extracting $RESOURCE_NAME"
    safeRunCommand "$EXTRACT_COMMAND"
    cd $DEP_NAME
    
    echo -e "\n### Configuring $RESOURCE_NAME"
    safeRunCommand "$CONFIGURE_COMMAND"
    
    echo -e "\n### Compiling $RESOURCE_NAME"
    safeRunCommand "$MAKE_COMMAND"
    
	echo -e "\n### Installing $RESOURCE_NAME"
    safeRunCommand "$INSTALL_COMMAND"
fi

echo -e "\nBuilding successfully completed.\nInstallation location: $INSTALL_PREFIX\n\n"

cd $originalPwd

exit 0

