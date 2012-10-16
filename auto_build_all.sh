#!/bin/bash

# Make sure we are in correct directory
cd $(dirname $0)

# Get some information about our system
OS=$(uname -s)
NUM_CORES=2
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=$(lsb_release -is)
    DISTRO_RELEASE=$(lsb_release -rs)
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
    if [ "$NUM_CORES" -gt "8" ]; then NUM_CORES=8; fi
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
fi

echo -e "\n\n### Fetching dependencies from ${DISTRO_ID}'s package management system. ###\n\n"
sleep 2

if [ "$DISTRO_ID" == "Ubuntu" ]; then
    sudo apt-get install subversion build-essential doxygen libqt4-dev libnss3-dev libnspr4-dev libreadline-dev libsqlite3-dev libX11-dev zlib1g-dev libXext-dev gfortran liblapack-dev
fi

echo -e "\n\n### Starting compilation (using $NUM_CORES threads) and packaging of the karaboFramework. ###\n\n"
sleep 2

cd build/netbeans/karabo
make -j$NUM_CORES package

echo -e "\n\n### Successfully finished building and packaging of karaboFramework. ###\n\n"

