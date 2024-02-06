#!/bin/bash

##############################################################################
# Packages that we know how to build

scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
source "$scriptDir/../set_lsb_release_info.sh"

##############################################################################
# Important constants

PYTHON_VERSION=3.11.6
PYTHON_PATH_VERSION=3.11
CONAN_RECIPE_CHANNEL=py311
BOOST_VERSION=1.82.0
LOG4CPP_VERSION=1.1.3
DAEMONTOOLS_VERSION=1.11-karabo3
OPENMQ_VERSION=5.1.3
OPENMQC_VERSION=5.1.4.1

##############################################################################
# Define a bunch of functions to be called later

check_for() {
    which $1 &> /dev/null
    if [ $? -ne 0 ]; then
        return 1
    fi

    # Installed and ready!
    return 0
}

get_abs_path() {
    local parent_dir=$(dirname "$1")
    local _basename=$(basename "$1")
    case $_basename in
    ..)
        cd "$1"
        local abs_path="$(pwd -P)"
        ;;
    *)
        cd "$parent_dir"
        local abs_path="$(pwd -P)"/"$_basename"
    esac
    cd - >/dev/null
    echo "$abs_path"
}

checkReturnCode() {
    ret_code=$?
    if [ $ret_code != 0 ]; then
        if [ -n "$1" ]; then
            # if the output is present, print it in case of error
            cat $1>&1
            # remove the temporary file before exiting
            rm -f $1
            # redirect the filedescriptor 3 to stdout as is tradition
            exec 3>&1
        fi
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        exit $ret_code
    fi
}

safeRunCommand() {
    typeset cmnd="$*"
    typeset ret_code

    echo cmnd=$cmnd
    eval $cmnd
    checkReturnCode ""
}

safeRunCommandQuiet() {
    local cmnd="$*"
    local ret_code

    tmp_output=$(mktemp)
    echo cmnd=$cmnd
    # redirect the stream with file descriptor 3 to the temporary file
    exec 3>"$tmp_output"
    # execute the command redirecting the output to 3
    eval $cmnd>&3 2>&3
    # `checkReturnCode` will print the output even in quiet mode.
    checkReturnCode $tmp_output
    # remove the temporary file to clean up
    rm -f $tmp_output
    # redirect the filedescriptor 3 to stdout as is tradition
    exec 3>&1
}

install_python() {
    pushd $scriptDir

    safeRunCommandQuiet "rm -rf $INSTALL_PREFIX/conan_toolchain-$TARGET_ARCH"

    # create default build profile
    safeRunCommandQuiet "conan profile new default --detect --force"
    # package_revision_mode is best: https://blog.conan.io/2019/09/27/package-id-modes.html
    safeRunCommandQuiet "conan config set general.revisions_enabled=1"
    safeRunCommandQuiet "conan config set general.default_package_id_mode=package_revision_mode"

    # python package opts
    local package_opts="./resources/python/conanfile.py cpython/$PYTHON_VERSION@karabo/$CONAN_RECIPE_CHANNEL"
    # configure prefix paths
    local folder_opts="--install-folder=$INSTALL_PREFIX/conan_toolchain-$TARGET_ARCH --output-folder=$INSTALL_PREFIX"
    # build python if not found in conan cache
    local build_opts="--build=missing"
    # apply custom profile on top of default profile
    local profile_opts="-pr:b=./conanprofile.karabo -pr:h=./conanprofile.karabo"
    # always compile patchelf, b2, openssl from source (needed later), ensures linkage against correct GLIBC version symbols
    if [[ $INSTALL_PREFIX == *"CentOS-7"* ]]; then
        safeRunCommandQuiet "conan install patchelf/0.13@ --build=patchelf --build=missing $profile_opts"
        safeRunCommandQuiet "conan install b2/4.9.6@ --build=b2 --build=missing $profile_opts"
        safeRunCommandQuiet "conan install openssl/1.1.1l@ --build=openssl --build=missing -o shared=True $profile_opts"
        safeRunCommandQuiet "conan install openssl/1.1.1l@ --build=openssl --build=missing -o shared=False $profile_opts"
    fi
    # copy conan recipe from extern/resources/python to local conan cache
    safeRunCommandQuiet "conan export $package_opts"
    # install packages listed in the extern/conanfile-bootstrap.txt
    safeRunCommandQuiet "conan install conanfile-bootstrap.txt $folder_opts $build_opts $profile_opts"

    # use pip in INSTALL_PREFIX by calling python3 -m pip <args>
    local pip_install_cmd="$INSTALL_PREFIX/bin/python3 -m pip install"

    # install python requirements
    # we do this in multiple stages, as pip has issues resolving a
    # too complex dependency chain with many pinned versions.
    safeRunCommandQuiet "$pip_install_cmd --force-reinstall -r requirements-pre.txt"
    # important dependencies that many other packages will need
    # (ie: numpy) are installed first
    safeRunCommandQuiet "$pip_install_cmd -r requirements-0.txt"
    # install everything else
    safeRunCommandQuiet "$pip_install_cmd -r requirements-1.txt"

    # fix rpaths
    safeRunCommand "./relocate_deps.sh $INSTALL_PREFIX"

    popd
}

install_from_deps() {
    pushd $scriptDir

        # create default build profile
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan profile new default --detect --force"

        # export local daemontools recipe
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan export ./resources/daemontools/conanfile.py daemontools-encore/$DAEMONTOOLS_VERSION@karabo/$CONAN_RECIPE_CHANNEL"
        # export local log4cpp recipe
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan export ./resources/log4cpp/conanfile.py log4cpp/$LOG4CPP_VERSION@karabo/$CONAN_RECIPE_CHANNEL"
        # export local boost recipe
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan export ./resources/boost/conanfile.py boost/$BOOST_VERSION@karabo/$CONAN_RECIPE_CHANNEL"
        # export local openmq/openmqc recipe
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan export ./resources/openmq/conanfile.py openmq/$OPENMQ_VERSION@karabo/$CONAN_RECIPE_CHANNEL"
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan export ./resources/openmqc/conanfile.py openmqc/$OPENMQC_VERSION@karabo/$CONAN_RECIPE_CHANNEL"

        # configure prefix paths
        local folder_opts="--install-folder=$INSTALL_PREFIX/conan_toolchain-$TARGET_ARCH --output-folder=$INSTALL_PREFIX"
        # when should conan build from sources? missing means if no pre-compiled binary package exists
        # boost:python_executable comes from a variable, so it must be defined here
        local build_opts="--build=missing -o boost:python_executable=$INSTALL_PREFIX/bin/python"
        # apply custom profile on top of default profile
        local profile_opts="-pr:b=./conanprofile.karabo -pr:h=./conanprofile.karabo"

        # install packages listed in the extern/conanfile.txt
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan install . $folder_opts $build_opts $profile_opts"
        # fix read-only files installed by nss (to enable conan reinstalls)
        safeRunCommandQuiet "chmod +w $INSTALL_PREFIX/include/*"

    # for whatever reason conan does not reliably copy *.pc files from its root directory
    # we do this here instead, and also capture any .pc files our from source builds created
    # in the process.
    safeRunCommand "mkdir -p $INSTALL_PREFIX/lib/pkgconfig/"
    cp $INSTALL_PREFIX/conan_toolchain-$TARGET_ARCH/*.pc $INSTALL_PREFIX/lib/pkgconfig/
    # now fix occurences of prefixes such that packages can use the "--define-prefix" option
    # of pkgconfig
    sed -i 's|prefix=.*|prefix=\${KARABO}/extern|g' $INSTALL_PREFIX/lib/pkgconfig/*.pc
    sed -i 's|libdir=.*|libdir=\${prefix}/lib|g' $INSTALL_PREFIX/lib/pkgconfig/*.pc
    sed -i 's|includedir=.*|includedir=\${prefix}/include|g' $INSTALL_PREFIX/lib/pkgconfig/*.pc
    sed -i 's|exec_prefix=.*|exec_prefix=\${prefix}|g' $INSTALL_PREFIX/lib/pkgconfig/*.pc

    popd
}

usage() {
    echo
    echo "Build the Karabo Dependencies"
    echo
    echo "Usage: $0 [args] INSTALL_DIRECTORY CI|ALL"
    echo "  INSTALL_DIRECTORY : The directory where build artifacts are installed"
    echo "  CI|ALL : The type of build to perform"
    echo "Arguments:"
    echo "  --package | -p : After building, make a tarball of the install directory"
    echo "  --quiet | -q : Suppress output of build commands"
    echo
}

##############################################################################
# We start executing here

# Parse command line args (anything starting with '-')
BUILD_PACKAGE="n"
QUIET=""
until [ ${1:0:1} != "-" ]; do
    case $1 in
        --package|-p)
        BUILD_PACKAGE="y"
        ;;
        --quiet|-q)
        QUIET="-q"
        ;;
        *)
        echo "Unrecognized argument '$1'"
        usage
        exit 1
        ;;
    esac
    shift
done

##############################################################################
# (Following all the arguments preceded by '-' or '--')
# $1  INSTALL_PREFIX  Installation prefix
# $2+ WHAT            Which packages to build (symbolic or specific)
EXTERN_DIR=$(dirname $0)
INSTALL_PREFIX=$(get_abs_path $1)
TARGET_ARCH=$(uname -m)
WHAT=${@:2}
FORCE="n"

# Make sure conan is available
check_for conan
if [ $? -ne 0 ]; then
    echo
    echo
    echo "!!! 'conan' command not found!"
    echo "Please install 'conan' so that dependencies can be downloaded!"
    echo
    echo
    # Give the user time to see the message
    sleep 2
    return 1
fi

# python download and install to allow full bootstrap
# install framework python dependencies via pip
check_for $INSTALL_PREFIX/bin/python
if [ $? -ne 0 ]; then
    install_python
fi

# install framework build dependencies via conan
install_from_deps

echo "### All external dependencies successfully installed/already present. ###"
