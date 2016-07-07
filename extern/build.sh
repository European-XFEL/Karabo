#!/bin/bash

element_in() {
    local e
    for e in ${@:2}
    do
        [[ "$e" == "$1" ]] && return 0
    done
    return 1
}

get_abs_path() {
    local PARENT_DIR=$(dirname "$1")
    local BASENAME=$(basename "$1")
    case $BASENAME in
    ..)
        cd "$1"
        local ABS_PATH="$(pwd -P)"
        ;;
    *)
        cd "$PARENT_DIR"
        local ABS_PATH="$(pwd -P)"/"$BASENAME"
    esac
    cd - >/dev/null
    echo "$ABS_PATH"
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

usage() {
    echo
    echo "Build the Karabo Dependencies"
    echo
    echo "Usage: $0 [args] INSTALL_DIRECTORY ALL|NOGUI|PYTHON|GUI|<list>"
    echo "  INSTALL_DIRECTORY : The directory where build artifacts are installed"
    echo "  ALL|NOGUI|PYTHON|GUI|<list> : The type of build to perform, OR"
    echo "                                <list> is a list of packages to build"
    echo "Arguments:"
    echo "  --package | -p : After building, make a tarball of the install directory"
    echo
}

##############################################################################

DEPENDENCIES_BASE=( bzip2 libpng snappy jpeg tiff python3.4 lapack boost
freetype hdf5 log4cpp cppunit openmq openmqc patchelf gmock )

DEPENDENCIES_PYTHON=( setuptools pip wheel cython numpy scipy nose pillow
sip backports tornado pyparsing six dateutil pytz pexpect pyzmq markupsafe
jinja2 pygments docutils alabaster babel snowballstemmer sphinx_rtd_theme
sphinx ipython h5py pyusb parse quamash suds jsonschema ecdsa pycrypto paramiko
tzlocal httplib2 pssh traits pint )

DEPENDENCIES_GUI=( qt4 pyqt4 matplotlib pyqwt5 guidata guiqwt )

DEPENDENCIES_DARWIN=( wheel pyqwt5 guidata guiqwt boost openmqc hdf5 h5py
log4cpp cppunit parse snappy traits pint )

##############################################################################
# Parse command line args (anything starting with '-')
BUILD_PACKAGE="n"
until [ ${1:0:1} != "-" ]; do
    case $1 in
        --package|-p)
        BUILD_PACKAGE="y"
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

case "$WHAT" in
    ALL)
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

MARKER_PATH=$INSTALL_PREFIX/.marker.txt

#check if $MARKER_PATH exists or not => if it does then read contents.
#Otherwise create the file and input names of packages one by one as they are installed.
if [ ! -f $MARKER_PATH ]; then touch $MARKER_PATH; fi

# Read the marker file into a variable as a list
IFS=$'\r\n' MARKER=$(cat $MARKER_PATH)
unset IFS

# Run the build from inside the extern directory
pushd $EXTERN_DIR

# Install packages one by one.
for i in "${DEPENDENCIES[@]}"
do
    :
    element_in "$i" "${MARKER[@]}"
    vin=$?
    if [ $vin -eq 0 -a "$FORCE" = "n" ]; then
        continue
    fi
    ./build_resource.sh $i $INSTALL_PREFIX
    rv=$?
    if [ $rv -ne 0 ]; then
        echo
        echo "### PROBLEMS building $i, exiting... ###"
        echo
        exit $rv
    fi
    echo $i >> $MARKER_PATH
done

# Package everything up
if [ "$BUILD_PACKAGE" = "y" ]; then
    VERSION=$(git rev-list --count HEAD)
    INSTALL_DIR=$(basename $INSTALL_PREFIX)
    pushd $(dirname $INSTALL_PREFIX)
    safeRunCommand "tar -zcf $INSTALL_PREFIX-$VERSION.tar.gz $INSTALL_DIR"
    popd
fi


popd
echo "### All external dependencies successfully installed/already present. ###"
