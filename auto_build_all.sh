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
    echo
    echo
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
Usage: $0 Debug|Release|Clean [flags]

Available flags:
  --auto      - Tries to automatically install needed system packages (sudo rights required!)
  --noBundle  - Only installs Karabo, does not create the software bundle

End-of-help

    exit 0
fi

# Fetch configuration type (Release or Debug)
if [[ $1 = "Release" || $1 = "Debug" ]]; then
	CONF=$1
elif [[ $1 = "Clean" ]]; then
    safeRunCommand "cd $scriptDir/build/netbeans/karabo"
    safeRunCommand "make bundle-clean CONF=Debug"
    safeRunCommand "make bundle-clean CONF=Release"
    safeRunCommand "cd $scriptDir/build/netbeans/karabo"
    rm -rf dist build nbproject/Makefile* nbproject/Package* nbproject/private
    safeRunCommand "cd $scriptDir/build/netbeans/karathon"
    rm -rf dist build nbproject/Makefile* nbproject/Package* nbproject/private
    safeRunCommand "cd $scriptDir/build/netbeans/deviceServer"
    rm -rf dist build nbproject/Makefile* nbproject/Package* nbproject/private
    safeRunCommand "cd $scriptDir/build/netbeans/brokerMessageLogger"
    rm -rf dist build nbproject/Makefile* nbproject/Package* nbproject/private
    safeRunCommand "cd $scriptDir"
    rm -rf package
    exit 0
else
	echo
	echo "Invalid option supplied. Allowed options: Release|Debug|Clean"
	echo
    exit 1
fi

# Check whether to build system dependencies
if [[ $2 = "--auto" || $3 = "--auto" ]]; then
    SKIP="n"
else 
    SKIP="y"
fi

# Check whether to skip bundling
if [[ $2 = "--noBundle" || $3 = "--noBundle" ]]; then
    BUNDLE="n"
else 
    BUNDLE="y"
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
    NUM_CORES=`sysctl hw.ncpu | awk '{print $2}'`
fi

# Cut the total number to ensure memory fitness
if [ "$NUM_CORES" -gt "10" ]; then NUM_CORES=10; fi

if [ "$SKIP" = "n" ]; then
    echo 
    echo "### Fetching dependencies from ${DISTRO_ID}'s package management system. ###"
    echo
    
    sleep 2
    
    # Platform specific sections here

	######################################
        #              Ubuntu                #
        ######################################


    if [ "$DISTRO_ID" == "Ubuntu" ]; then
        safeRunCommand "sudo apt-get install subversion build-essential doxygen libqt4-dev libnss3-dev libnspr4-dev libreadline-dev libsqlite3-dev libqt4-sql-sqlite libX11-dev zlib1g-dev gfortran liblapack-dev m4 libssl-dev"
        if [ "$DISTRO_RELEASE" = "10.04" ]; then
            safeRunCommand "sudo apt-get install libxext-dev"
        fi
        safeRunCommand "sudo apt-get install krb5-user"

	######################################
        #    Scientific Linux or CentOS      #
        ######################################

    elif [ "$DISTRO_ID" == "Scientific" ]; then
        safeRunCommand "yum install redhat-lsb"
        safeRunCommand "yum install make gcc gcc-c++ gcc-gfortran subversion doxygen nspr-devel nss-devel zlib-devel libX11-devel readline-devel qt-devel lapack-devel sqlite-devel openssl-devel"
        safeRunCommand "yum install epel-release
    yum --enablerepo=epel install qtwebkit-devel"
        safeRunCommand "yum install krb5-workstation"


	######################################
        #              MacOSX                #
        ######################################

    elif [ "$DISTRO_ID" == "MacOSX" ]; then	
	echo "### This can take a while. Better prepare yourself a coffee..."
	safeRunCommand "sudo port install nspr nss pkgconfig sqlite3 python27 py27-numpy py27-scipy py27-matplotlib py27-pyqt4 py27-pyqwt py27-zmq py27-tornado  py27-pygments py27-nose py27-ipython"
	safeRunCommand "sudo port select --set python python27"
	safeRunCommand "sudo port select --set ipython ipython27" 
	safeRunCommand "sudo easy_install readline"
        # Patch reported macports bug (#37201)
	safeRunCommand "sudo cp -rf extern/resources/bundleMacOSX/sqldrivers /opt/local/share/qt4/plugins"
        # Patch NetBeans bug regarding Makefile pathes
	safeRunCommand "cd /usr/bin"
	safeRunCommand "sudo ln -sf /opt/local/bin/pkg-config pkg-config"
	safeRunCommand "cd -"
    fi
fi

echo
echo "### Starting compilation (using $NUM_CORES threads) and packaging of the karaboFramework. ###"
echo

sleep 2

safeRunCommand "cd $scriptDir/build/netbeans/karabo"

if [ "$BUNDLE" = "y" ]; then

    safeRunCommand "make CONF=$CONF -j$NUM_CORES bundle-package"
    
else
    
    safeRunCommand "make CONF=$CONF -j$NUM_CORES bundle-install"

fi

echo "### Successfully finished building and packaging of karaboFramework ###"
echo
exit 0
