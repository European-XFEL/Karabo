#!/bin/bash

# Script for automated building and packaging of the entire karaboFramework
#
# Author: <burkhard.heisen@xfel.eu>
#

# Help function for checking successful execution of commands
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

# Make sure the script runs in the correct directory
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
cd ${scriptDir}
if [ $? -ne 0 ]; then
    echo " Could not change directory to ${scriptDir}"
    exit 1;
fi

# Parse command line
if [[ -z "$1" ||  $1 = "help" || $1 = "-h" ||  $1 = "-help" || $1 = "--help" ]]; then
    cat <<End-of-help
Usage: $0 Debug|Release [flags]

Available flags:
  --auto      - Tries to automatically install needed system packages (sudo rights required!)

End-of-help

    exit 0
fi

# Fetch configuration type (Release or Debug)
if [[ $1 = "Release" || $1 = "Debug" ]]; then
	CONF=$1
else
	echo
	echo "Invalid option supplied. Allowed options: Release|Debug"
	echo
    exit 1
fi
# Check whether to skip sys deps
if [[ $2 = "--auto" ]]; then
    SKIP="n"
else 
    SKIP="y"
fi

# Get some information about our system
OS=$(uname -s)
NUM_CORES=2
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    NUM_CORES=`grep "processor" /proc/cpuinfo | wc -l`
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    CONF=${CONF}-MacOSX
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

# Cut the total number to ensure memory fitness
if [ "$NUM_CORES" -gt "8" ]; then NUM_CORES=8; fi

if [ "$SKIP" = "n" ]; then
    echo 
    echo "### Fetching dependencies from ${DISTRO_ID}'s package management system. ###"
    echo
    
    sleep 2
    
    # Platform specific sections here
    if [ "$DISTRO_ID" == "Ubuntu" ]; then
        safeRunCommand "sudo apt-get install subversion build-essential doxygen libqt4-dev libnss3-dev libnspr4-dev libreadline-dev libsqlite3-dev libqt4-sql-sqlite libX11-dev zlib1g-dev gfortran liblapack-dev m4 libssl-dev"
        if [ "$DISTRO_RELEASE" = "10.04" ]; then
            safeRunCommand "sudo apt-get install libxext-dev"
        fi
        safeRunCommand "sudo apt-get install krb5-user"
    elif [ "$DISTRO_ID" == "Scientific" ]; then
        safeRunCommand "yum install redhat-lsb"
        safeRunCommand "yum install make gcc gcc-c++ gcc-gfortran subversion doxygen nspr-devel nss-devel zlib-devel libX11-devel readline-devel qt-devel lapack-devel sqlite-devel openssl-devel"
        safeRunCommand "yum install epel-release
    yum --enablerepo=epel install qtwebkit-devel"
        safeRunCommand "yum install krb5-workstation"
    fi
fi

echo
echo "### Starting compilation (using $NUM_CORES threads) and packaging of the karaboFramework. ###"
echo

sleep 2

safeRunCommand "cd build/netbeans/karabo"
safeRunCommand "make CONF=$CONF -j$NUM_CORES"
safeRunCommand "make CONF=$CONF bundle-package"

echo "### Successfully finished building and packaging of karaboFramework ###"
echo
exit 0
