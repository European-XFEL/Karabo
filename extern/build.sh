#!/bin/bash

##############################################################################
# Packages that we know how to build

scriptDir=$(dirname $([[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"))
source "$scriptDir/../set_lsb_release_info.sh"

##############################################################################
# Important constants

CONAN_RECIPE_CHANNEL=py312
DAEMONTOOLS_VERSION=1.11-karabo3
NSS_VERSION=3.93

declare -A CONAN_MIRRORS=(
    ["GNU_DOGADO"]="http://mirror.dogado.de/gnu/"
    ["GNU_FAU"]="http://ftp.fau.de/gnu"
)

##############################################################################
# Define a bunch of functions to be called later

check_for() {
    which $1 &>/dev/null
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
        ;;
    esac
    cd - >/dev/null
    echo "$abs_path"
}

checkReturnCode() {
    ret_code=$?
    if [ $ret_code != 0 ]; then
        if [ -n "$1" ]; then
            # if the output is present, print it in case of error
            cat $1 >&1
            # remove the temporary file before exiting
            rm -f $1
            # redirect the filedescriptor 3 to stdout as is tradition
            exec 3>&1
        fi
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        exit $ret_code
    fi
}

add_conan_mirrors() {
    # Setup mirrors for conan to look for
    for mirror in "${!CONAN_MIRRORS[@]}"; do
        mirror_url="${CONAN_MIRRORS[$mirror]}"
        if conan remote list | grep -q "$mirror_url"; then
            echo "Remote $mirror is already added."
        else
            echo "Adding remote $mirror with URL $mirror_url"
            safeRunCommandQuiet conan remote add "$mirror" "$mirror_url"
        fi
    done
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
    eval $cmnd >&3 2>&3
    # `checkReturnCode` will print the output even in quiet mode.
    checkReturnCode $tmp_output
    # remove the temporary file to clean up
    rm -f $tmp_output
    # redirect the filedescriptor 3 to stdout as is tradition
    exec 3>&1
}

install_python() {
    pushd $scriptDir

    safeRunCommandQuiet "rm -rf $INSTALL_PREFIX/conan_toolchain"

    # create default build profile
    safeRunCommandQuiet "conan profile detect --force"

    add_conan_mirrors
    # configure prefix paths
    local folder_opts="--deployer=karabo_deployer --deployer-folder=$INSTALL_PREFIX --output-folder=$INSTALL_PREFIX/conan_toolchain"
    # build python if not found in conan cache
    local build_opts="--build=missing"
    # apply custom profile on top of default profile
    local profile_opts="-pr:h=./conanprofile.karabo"
    # always compile patchelf, b2, openssl from source (needed later), ensures linkage against correct GLIBC version symbols
    # (Still needed after removal of opemq(c)?)
    build_opts="$build_opts -o openssl/*:openssldir=/etc/ssl"
    # install packages listed in the extern/conanfile-bootstrap.txt
    if [[ $INSTALL_PREFIX == *"Ubuntu-20"* ]]; then
        # Work around a failure for building the conan package "cpython/3.12.2"
        # on Ubuntu 20. For an unknown reason the "configure" that ships with the
        # cpython source package fails to check that the function "shm_open" is
        # exported by "librt.so". The same "configure" works as expected on the
        # same Ubuntu 20 system during a manual installation of cpython from source.
        # Details at https://git.xfel.eu/Karabo/Framework/-/merge_requests/8551#note_467905
        export POSIXSHMEM_LIBS="-lrt"
    fi

    # Make sure that the new URL for Conan Center remote is used. The previous one, center.conan.io,
    # stopped receiving updates since November, 4th, 2024.
    # Full details at:
    # https://blog.conan.io/2024/09/30/Conan-Center-will-stop-receiving-updates-for-Conan-1.html
    # NOTE: The conditional update of the Conan Center remote in the next lines can be removed if
    #       all installations are guaranteed to be using at least conan 2.9.2
    conan_center_url=$(conan remote list | grep "conancenter" | awk '{print $2}')
    # If the URL is outdated, update it
    if [[ "$conan_center_url" == "https://center.conan.io" ]]; then
        safeRunCommandQuiet "conan remote update conancenter --url https://center2.conan.io"
    fi

    safeRunCommandQuiet "conan install conanfile-bootstrap.txt $folder_opts $build_opts $profile_opts"

    # ensure that python can always find its libpython.so
    safeRunCommand "$INSTALL_PREFIX/bin/patchelf --force-rpath --set-rpath '\$ORIGIN/../lib' $INSTALL_PREFIX/bin/python3.12"

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

    popd
}

install_from_deps() {
    pushd $scriptDir

    # create default build profile
    safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan profile detect --force"

        add_conan_mirrors
        # export local conan recipes (for packages where no public recipe exists
        # we keep custom conan recipes in extern/resources/<pkg_name>)
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan export ./resources/daemontools/conanfile.py --name daemontools-encore --version $DAEMONTOOLS_VERSION --user karabo --channel $CONAN_RECIPE_CHANNEL"
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan export ./resources/nss/conanfile.py --name nss --version $NSS_VERSION --user karabo --channel $CONAN_RECIPE_CHANNEL"

    # configure prefix paths
    local folder_opts="--deployer=karabo_deployer --deployer-folder=$INSTALL_PREFIX --output-folder=$INSTALL_PREFIX/conan_toolchain"
    # when should conan build from sources? missing means if no pre-compiled binary package exists
    local build_opts="--build=missing"
    # apply custom profile on top of default profile
    local profile_opts="-pr:h=./conanprofile.karabo"

    # always compile openssl from source (needed later), ensures linkage against correct GLIBC version symbols
    # (Still needed after removal of openmq(c)?)
    build_opts="$build_opts -o openssl/*:openssldir=/etc/ssl"

    # install packages listed in the extern/conanfile.txt
    safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan install . $folder_opts $build_opts $profile_opts"
    # fix read-only files installed by nss (to enable conan reinstalls)
    safeRunCommandQuiet "chmod +w $INSTALL_PREFIX/include/*"

    # fix rpaths
    # Relocate the libraries/executables
    safeRunCommand "find $INSTALL_PREFIX/lib -maxdepth 1 -name '*.so' -exec $INSTALL_PREFIX/bin/patchelf --force-rpath --set-rpath '\$ORIGIN/../lib' {} \;"
    safeRunCommandQuiet "./relocate_deps.sh $INSTALL_PREFIX"

    # for whatever reason conan does not reliably copy *.pc files from its root directory
    # we do this here instead, and also capture any .pc files our from source builds created
    # in the process.
    safeRunCommandQuiet "mkdir -p $INSTALL_PREFIX/lib/pkgconfig/"
    cp $INSTALL_PREFIX/conan_toolchain/*.pc $INSTALL_PREFIX/lib/pkgconfig/
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
    --package | -p)
        BUILD_PACKAGE="y"
        ;;
    --quiet | -q)
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
else
   # Make aware of which conan is used
   conan --version
fi

# Some dependencies, like libpng specify minimum required versions of cmake to be older than
# "3.5" - starting with cmake "4.X", the minimum cmake version must be at least "3.5".
# Details at https://cmake.org/cmake/help/v4.0/release/4.0.html#id14
# Setting the environment variable below allows passing the cmake check.
export CMAKE_POLICY_VERSION_MINIMUM="3.5"

# python download and install to allow full bootstrap
# install framework python dependencies via pip
check_for $INSTALL_PREFIX/bin/python
if [ $? -ne 0 ]; then
    install_python
fi

# install framework build dependencies via conan
install_from_deps

echo "### All external dependencies successfully installed/already present. ###"
