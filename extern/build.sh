#!/bin/bash

##############################################################################
# Packages that we know how to build

DEPENDENCIES_BASE=( bzip2 libpng snappy jpeg tiff python3.4 lapack boost
freetype hdf5 log4cpp cppunit openmq openmqc patchelf gmock existDB )

DEPENDENCIES_PYTHON=( setuptools pip wheel cython numpy scipy six nose py pytest
pillow sip backports backports_abc tornado pyparsing dateutil pytz pexpect pyzmq
markupsafe jinja2 pygments docutils alabaster babel snowballstemmer
sphinx_rtd_theme imagesize sphinx decorator ipykernel ipython_genutils
jupyter_core jupyter_client pickleshare wcwidth prompt_toolkit simplegeneric
traitlets dill ipython h5py pyusb parse suds jsonschema ecdsa pycrypto paramiko
tzlocal httplib2 pssh traits pint nbformat notebook ipyparallel ipcluster_tools
cycler pyelftools rpathology )

DEPENDENCIES_GUI=( qt4 pyqt4 matplotlib qtconsole pyqwt5 guidata guiqwt )

DEPENDENCIES_DARWIN=( wheel pyqwt5 guidata guiqwt boost openmqc hdf5 h5py
log4cpp cppunit parse snappy traits pint )

##############################################################################
# Important constants

BUILD_MARKER_NAME=".marker.txt"
DEPS_MARKER_NAME=".deps_tag.txt"
DEP_URL_BASE="http://exflserv05.desy.de/karabo/karaboDevelopmentDeps"
DEPS_BASE_NAME=$(lsb_release -is)-$(lsb_release -rs | sed -r 's/^([0-9]+).*/\1/')
DEP_TAG_PATTERN="deps-*"

##############################################################################
# Define a bunch of functions to be called later

build_dependencies() {
    local built_deps=$(get_built_dependencies)
    local marker_path=$INSTALL_PREFIX/$BUILD_MARKER_NAME

    # Run the build from inside the extern directory
    pushd $EXTERN_DIR

    # Install packages one by one.
    for i in "${DEPENDENCIES[@]}"
    do
        :
        element_in "$i" "${built_deps[@]}"
        local vin=$?
        if [ $vin -eq 0 -a "$FORCE" = "n" ]; then
            continue
        fi
        ./build_resource.sh $QUIET $i $INSTALL_PREFIX
        local rv=$?
        if [ $rv -ne 0 ]; then
            echo
            echo "### PROBLEMS building $i, exiting... ###"
            echo
            exit $rv
        fi
        echo $i >> $marker_path
    done

    # Leave the extern directory
    popd
}

check_for_curl() {
    which curl &> /dev/null
    if [ $? -ne 0 ]; then
        echo
        echo
        echo "!!! 'curl' command not found!"
        echo "Please install 'curl' so that dependencies can be downloaded!"
        echo
        echo
        # Give the user time to see the message
        sleep 2
        return 1
    fi

    # Installed and ready!
    return 0
}

check_if_download_needed() {
    local deps_tag=$(get_last_deps_tag)

    if [ "$deps_tag" = "" ]; then
        # This branch has no dependencies tag. Use old build logic.
        return 2
    fi

    local build_marker_path=$INSTALL_PREFIX/$BUILD_MARKER_NAME
    if [ -f $build_marker_path ]; then
        # This branch was built using the old system. Use old build logic
        return 2
    fi

    local deps_marker_path=$INSTALL_PREFIX/$DEPS_MARKER_NAME
    if [ ! -f $deps_marker_path ]; then
         # Clean directory. The dependencies should be downloaded
        return 1
    fi

    local installed_tag=$(cat $deps_marker_path)
    if [ $deps_tag != $installed_tag ]; then
        # Wrong dependency version. The dependencies should be downloaded
        return 1
    fi

    # The correct dependencies are already installed
    return 0
}

determine_build_packages() {
    case "$WHAT" in
        CI|ALL)
            # Build everything
            DEPENDENCIES=( ${DEPENDENCIES_BASE[@]} ${DEPENDENCIES_PYTHON[@]}
                           ${DEPENDENCIES_GUI[@]} )
            ;;
        NOGUI)
            # The classic NOGUI option
            DEPENDENCIES=( ${DEPENDENCIES_BASE[@]} ${DEPENDENCIES_PYTHON[@]} )
            ;;
        DARWIN)
            # Mac OS X dependencies
            DEPENDENCIES=( ${DEPENDENCIES_DARWIN[@]} )
            ;;
        PYTHON)
            # Python packages
            DEPENDENCIES=( ${DEPENDENCIES_PYTHON[@]} )
            ;;
        GUI)
            # GUI-related packages
            DEPENDENCIES=( ${DEPENDENCIES_GUI[@]} )
            ;;
        *)
            # FORCE building of whatever the user specified.
            DEPENDENCIES=( $WHAT )
            FORCE="y"
            ;;
    esac
}

download_latest_deps() {
    local deps_tag=$(get_last_deps_tag)
    local deps_file=$DEPS_BASE_NAME-$deps_tag.tar.gz
    local deps_url=$DEP_URL_BASE/$deps_file

    echo "### Downloading external dependencies from $deps_url. ###"
    echo

    # Make sure curl is available
    check_for_curl
    if [ $? -ne 0 ]; then
        return 1
    fi

    # Attempt to download, quietly
    curl -fO $deps_url &> /dev/null
    if [ $? -ne 0 ]; then
        if [ -f $deps_file ]; then
            rm $deps_file
        fi

        echo "Fetching $deps_url failed!"
        return 1
    fi

    # Unpack
    tar -zxf $deps_file
    rm $deps_file
    mv $DEPS_BASE_NAME $INSTALL_PREFIX

    # Adjust for the local environment
    echo "### Updating dependencies for the local environment... (this takes a while) ###"
    echo

    pushd $EXTERN_DIR
    safeRunCommand "./relocate_deps.sh $INSTALL_PREFIX"
    popd

    # Leave a marker for later
    echo $deps_tag > $INSTALL_PREFIX/$DEPS_MARKER_NAME
    return 0
}

element_in() {
    local e
    for e in ${@:2}
    do
        [[ "$e" == "$1" ]] && return 0
    done
    return 1
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

get_built_dependencies() {
    mkdir -p $INSTALL_PREFIX
    local marker_path=$INSTALL_PREFIX/$BUILD_MARKER_NAME

    # Check if $marker_path exists or not => if it does then read contents.
    # Otherwise create the file and input names of packages one by one as they are installed.
    if [ ! -f $marker_path ]; then
        touch $marker_path
    fi

    # Read the marker file into a variable as a list
    local built_deps=""
    IFS=$'\r\n' built_deps=$(cat $marker_path)
    unset IFS

    echo $built_deps
}

get_last_deps_tag() {
    # Do a fast check for our special tag
    local has_tag=$(git tag -l "$DEP_TAG_PATTERN")
    if [ "$has_tag" = "" ]; then
        # Bail out early
        echo "" && return 1
    fi

    # Step through commits in reverse chronological order
    for rev in $(git rev-list HEAD)
    do
        local tagname=$(git tag -l "$DEP_TAG_PATTERN" --points-at $rev)
        # Once we find a commit with a "deps-*" tag, we're done.
        if [ "$tagname" != "" ]; then
            echo $tagname && return 0
        fi
    done

    # No tag was found
    echo "" && return 1
}

package_deps_directory() {
    local version=$(get_last_deps_tag)
    local install_dir=$(basename $INSTALL_PREFIX)
    pushd $(dirname $INSTALL_PREFIX)
    safeRunCommand "tar -zcf $install_dir-$version.tar.gz $install_dir"
    popd
}

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

try_dependency_download() {
    check_if_download_needed
    local dl_needed=$?
    if [ $dl_needed -eq 1 ]; then
        # No. Try to download them
        download_latest_deps
        if [ $? -eq 0 ]; then
            echo "### All external dependencies successfully installed from download. ###"
            exit 0
        else
            echo "No pre-built dependencies could be downloaded for this branch."
            echo "External dependencies will be built from source."
            echo
        fi
    elif [ $dl_needed -eq 0 ]; then
        echo "### All external dependencies already installed from download. ###"
        exit 0
    fi
}

usage() {
    echo
    echo "Build the Karabo Dependencies"
    echo
    echo "Usage: $0 [args] INSTALL_DIRECTORY CI|ALL|NOGUI|PYTHON|GUI|<list>"
    echo "  INSTALL_DIRECTORY : The directory where build artifacts are installed"
    echo "  CI|ALL|NOGUI|PYTHON|GUI|<list> : The type of build to perform, OR"
    echo "                                   <list> is a list of packages to build"
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
WHAT=${@:2}
FORCE="n"

if [ "$(uname -s)" = "Darwin" ]; then
    # No choice on OS X
    WHAT="DARWIN"
fi

if [ "$WHAT" = "ALL" ]; then
    # Attempt to install dependencies from FTP
    try_dependency_download
fi

# We only continue if nothing was downloaded

# What have we been asked to build?
determine_build_packages

# Build the dependencies
build_dependencies

# Package everything up, if requested
if [ "$BUILD_PACKAGE" = "y" ]; then
    package_deps_directory
fi

echo "### All external dependencies successfully installed/already present. ###"
