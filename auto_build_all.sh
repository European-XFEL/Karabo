#!/bin/bash

# Make sure we are in correct directory
cd $(dirname $0)

# Get some information about our system
OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=$(lsb_release -is)
    DISTRO_RELEASE=$(lsb_release -rs)
    tmp=`grep "processor" /proc/cpuinfo | wc -l`
    NUM_CORES=$(($tmp*2/3))
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    NUM_CORES=2
fi

echo -e "\n\n### Fetching dependencies from ${DISTRO_ID}'s package management system. ###\n\n"
sleep 2

if [ "$DISTRO_ID" == "Ubuntu" ]; then
    sudo apt-get install subversion krb5-user build-essential doxygen libqt4-dev libnss3-dev libnspr4-dev libreadline-dev libsqlite3-dev libX11-dev zlib1g-dev libXext-dev gfortran liblapack-dev
fi

echo -e "\n\n### Starting compilation and packaging of the karaboFramework.\n\n ###"
sleep 2

cd build/netbeans/karabo
make -j$NUM_CORES package

echo -e "\n\n### Successfully finished building and packaging of karaboFramework. ###\n\n"

