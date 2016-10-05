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

checkCppUnitTestResults() {
    local testDir="$1"
    for i in $testDir/testresults/*.xml; do
        # xmllint is distributed with Karabo
        local fails=$(xmllint --xpath '/TestRun/Statistics/FailuresTotal/text()' $i)
        if [ $fails != "0" ]; then
            echo "Test failure found in results file: $i"
            echo
            echo
            exit 1
        fi
    done
}

runIntegrationTests() {
    if [ -z "$KARABO" ]; then
        source $scriptDir/karabo/activate
    fi

    local testDir=$scriptDir/build/netbeans/integrationTests

    echo
    echo Running Karabo integration tests ...
    echo
    cd $testDir
    safeRunCommand "make CONF=$CONF -j$NUM_JOBS test"
    cd $scriptDir

    # Parse the XML test outputs
    checkCppUnitTestResults "$testDir"
}

runUnitTests() {
    if [ -z "$KARABO" ]; then
        source $scriptDir/karabo/activate
    fi

    local testDir=$scriptDir/build/netbeans/karabo

    echo
    echo Running Karabo unit tests ...
    echo
    cd $testDir
    safeRunCommand "make CONF=$CONF -j$NUM_JOBS test"
    cd $scriptDir

    # Parse the XML test outputs
    checkCppUnitTestResults "$testDir"

    echo
    echo Running pythonKarabo tests
    echo
    cd build/netbeans/pythonKarabo
    safeRunCommand "nosetests -v karabo.bound_api"
    safeRunCommand "nosetests -v karabo.common"
    safeRunCommand "nosetests -v karabo.middlelayer_api"
    safeRunCommand "nosetests -v karabo.tests"
    echo
    echo Unit tests complete
    echo
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
Usage: $0 Debug|Release|Dependencies|Clean|Clean-All [flags]

Available flags:
  --auto       - Tries to automatically install needed system packages (sudo rights required!)
  --noBundle   - Only installs Karabo, does not create the software bundle
  --pyDevelop  - Install Python packages in development mode rather than from wheels
  --runTests   - Run unit tests after building (useful for Debug|Release)
  --numJobs N  - Specify the number of jobs that make should use to run simultaneously

Note: "Dependencies" builds only the external dependencies
      "Clean" cleans all Karabo code (src folder)
      "Clean-All" additionally cleans all external dependencies (extern folder)

End-of-help

    exit 0
fi

EXTERN_ONLY="n"

# Fetch configuration type (Release or Debug)
if [[ $1 = "Release" || $1 = "Debug" ]]; then
    CONF=$1
elif [[ $1 = "Clean" || $1 = "Clean-All" ]]; then
    safeRunCommand "cd $scriptDir/build/netbeans/karabo"
    safeRunCommand "make bundle-clean CONF=Debug"
    safeRunCommand "make bundle-clean CONF=Release"
    if [[ $1 = "Clean-All" ]]; then 
        safeRunCommand "make clean-extern"
    fi
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
elif [[ $1 = "Dependencies" ]]; then
    echo "Building external dependencies"
    EXTERN_ONLY="y"
else
    echo
    echo "Invalid option supplied. Allowed options: Release|Debug|Dependencies|Clean|Clean-All"
    echo
    exit 1
fi

# Get rid of the first argument
shift

# Parse the commandline flags
SKIP="y"
BUNDLE="y"
RUNTESTS="n"
RUNINTEGRATIONTESTS="n"
PYOPT="wheel"
NUM_JOBS=0
while [ -n "$1" ]; do
    case "$1" in
        --auto)
            # Don't skip building system dependencies
            SKIP="n"
            ;;
        --noBundle)
            # Don't skip bundling
            BUNDLE="n"
            ;;
        --pyDevelop)
            # Build Python packages in development mode
            PYOPT="develop"
            ;;
        --runTests)
            # Run all the unit tests too
            RUNTESTS="y"
            ;;
        --runIntegrationTests)
            # Run the integration tests
            RUNINTEGRATIONTESTS="y"
            ;;
        --numJobs)
            # Limit the numbers of jobs for make runs
            if [ -n "$2" ]; then
                NUM_JOBS=$2
                shift
            else
                echo "Option --numJobs needs a number of jobs"
                exit 1
            fi
            ;;
        *)
            # Make a little noise
            echo "Unrecognized commandline flag: $1"
            ;;
    esac
    shift
done

# Get some information about our system
OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    DISTRO_ID=( $(lsb_release -is) )
    DISTRO_RELEASE=$(lsb_release -rs)
    if [ "$NUM_JOBS" = "0" ]; then
	NUM_JOBS=`grep "processor" /proc/cpuinfo | wc -l`
    fi
elif [ "$OS" = "Darwin" ]; then
    DISTRO_ID=MacOSX
    DISTRO_RELEASE=$(uname -r)
    if [ "$NUM_JOBS" = "0" ]; then
	NUM_JOBS=`sysctl hw.ncpu | awk '{print $2}'`
    fi
fi

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

    elif [ "$DISTRO_ID" == "Scientific" -o "$DISTRO_ID" == "CentOS" ]; then
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
        safeRunCommand "sudo port -v selfupdate || true"
        safeRunCommand "sudo port selfupdate || true"
        safeRunCommand "sudo port upgrade outdated || true"
        safeRunCommand "sudo port install nspr nss pkgconfig sqlite3 python34 py34-numpy py34-scipy py34-matplotlib py34-pyqt4 py34-zmq py34-tornado  py34-pygments py34-nose py34-ipython"
        safeRunCommand "sudo port select --set python python34"
        safeRunCommand "sudo port select --set ipython py34-ipython"
        safeRunCommand "sudo port install py34-readline"
        safeRunCommand "sudo port install py34-setuptools py34-pip"
        # Patch reported macports bug (#37201)
        #safeRunCommand "sudo cp -rf extern/resources/bundleMacOSX/sqldrivers /opt/local/share/qt4/plugins"
        # Patch NetBeans bug regarding Makefile pathes

        safeRunCommand "cd /usr/bin"
        safeRunCommand "sudo ln -sf /opt/local/bin/pkg-config pkg-config"
        safeRunCommand "cd -"

        safeRunCommand "cd /opt/local/Library/Frameworks/Python.framework/Versions/Current/include"
        safeRunCommand "sudo ln -sf python3.4m python3.4"
        safeRunCommand "cd -"
    fi
fi

echo
echo "### Starting compilation (using $NUM_JOBS jobs) and packaging of the karaboFramework. ###"
echo

sleep 2

safeRunCommand "cd $scriptDir/build/netbeans/karabo"

if [ $EXTERN_ONLY = "y" ]; then
    safeRunCommand "make -j$NUM_JOBS extern"
    if [ "$BUNDLE" = "y" ]; then
        safeRunCommand "make -j$NUM_JOBS package-extern"
    fi
elif [ "$BUNDLE" = "y" ]; then
    safeRunCommand "make CONF=$CONF PYOPT=$PYOPT -j$NUM_JOBS bundle-package"
else
    safeRunCommand "make CONF=$CONF PYOPT=$PYOPT -j$NUM_JOBS bundle-install"
fi
if [ "$RUNTESTS" = "y" ]; then
    runUnitTests
fi
if [ "$RUNINTEGRATIONTESTS" = "y" ]; then
    runIntegrationTests
fi

echo "### Successfully finished building and packaging of karaboFramework ###"
echo
exit 0
