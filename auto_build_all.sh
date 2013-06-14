#!/bin/bash

# Script for automated building and packaging of the entire karaboFramework
#
# Author: <burkhard.heisen@xfel.eu>
#

# Make sure the script runs in the correct directory
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
cd ${scriptDir}
if [ $? -ne 0 ]; then
    echo " Could not change directory to ${scriptDir}"
    exit 1;
fi

# Fetch configuration type (Release or Debug)
if [ $# -gt 1 ]; then
    if [ $1 = "Release" -o $1 = "Debug" ]
	CONF=$1
    else
	echo
	echo "Invalid option supplied. Allowed options: [Release|Debug]"
	echo
    fi
else
CONF="Debug" # default
fi

# Get some information about our system
OS=$(uname -s)
NUM_CORES=2
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
    if [ "$NUM_CORES" -gt "8" ]; then NUM_CORES=8; fi
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    CONF=${CONF}-MacOSX
fi

echo 
echo "### Fetching dependencies from ${DISTRO_ID}'s package management system. ###"
echo

sleep 2

# Platform specific sections here
if [ "$DISTRO_ID" == "Ubuntu" ]; then
    sudo apt-get install subversion build-essential doxygen libqt4-dev libnss3-dev libnspr4-dev libreadline-dev libsqlite3-dev libX11-dev zlib1g-dev libXext-dev gfortran liblapack-dev
elif [ "$DISTRO_ID" == "Scientific" ]; then
    echo "### install necessary devel packages: ###\n yum install sqlite-devel sqlite \n yum --enablerepo=epel install qtwebkit-devel"
fi

echo
echo "### Starting compilation (using $NUM_CORES threads) and packaging of the karaboFramework. ###"
echo

sleep 2

cd build/netbeans/karabo
make CONF=$CONF -j$NUM_CORES 
make CONF=$CONF package 

echo "### Successfully finished building and packaging of karaboFramework. ###"

