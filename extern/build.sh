#!/bin/bash

##############################################################################
# Packages that we know how to build

# dependencies located in extern/resources/. to manually compile and install
DEPENDENCIES_BASE=(  )

scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
source "$scriptDir/../set_lsb_release_info.sh"

##############################################################################
# Important constants

BUILD_MARKER_NAME=".marker.txt"
DEPS_MANAGER_MARKER_NAME=".conan.pip.marker.txt"
DEPS_MARKER_NAME=".deps_tag.txt"
if [ "${KARABO_UPLOAD_CURL_PREFIX}" == "" ]; then
    KARABO_UPLOAD_CURL_PREFIX=http://exflctrl01.desy.de/karabo
fi
DEP_URL_BASE="${KARABO_UPLOAD_CURL_PREFIX}/karaboDevelopmentDeps"
DEPS_OS_IDENTIFIER=$LSB_RELEASE_DIST$(echo $LSB_RELEASE_VERSION | sed -r 's/^([0-9]+).*/\1/')
DEP_TAG_PATTERN="deps-*"

declare -A DEPS_BASE_NAME_MAP
DEPS_BASE_NAME_MAP['Ubuntu18']='Ubuntu-18'
DEPS_BASE_NAME_MAP['Ubuntu19']='Ubuntu-18'
DEPS_BASE_NAME_MAP['Ubuntu20']='Ubuntu-20'
DEPS_BASE_NAME_MAP['Ubuntu22']='Ubuntu-22'
DEPS_BASE_NAME_MAP['CentOS7']='CentOS-7'
DEPS_BASE_NAME_MAP['AlmaLinux8']='AlmaLinux-8'
DEPS_BASE_NAME_MAP['AlmaLinux9']='AlmaLinux-9'
DEPS_BASE_NAME_MAP['Debian10']='Debian-10'

PYTHON_VERSION=3.11.6
PYTHON_PATH_VERSION=3.11
CONAN_RECIPE_CHANNEL=py311
BOOST_VERSION=1.82.0
LOG4CPP_VERSION=1.1.3
DAEMONTOOLS_VERSION=1.11-karabo3
OPENMQ_VERSION=5.1.3
OPENMQC_VERSION=5.1.4.1
CPP_STD_LIB_VERSION=c++11
CPP_STD=14

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

check_for_conan() {
    which conan &> /dev/null
    if [ $? -ne 0 ]; then
        echo
        echo
        echo "!!! 'conan 1.x' command not found!"
        echo "Please install latest 'conan 1.x' so that dependencies can be downloaded!"
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
            DEPENDENCIES=( ${DEPENDENCIES_BASE[@]} )
            ;;
        *)
            # FORCE building of whatever the user specified.
            DEPENDENCIES=( $WHAT )
            FORCE="y"
            ;;
    esac
}

download_latest_deps() {

    local deps_base_name=${DEPS_BASE_NAME_MAP[$DEPS_OS_IDENTIFIER]}

    if [ -z $deps_base_name ]; then
        echo
        echo "Failed to download the pre-compiled external dependencies because"
        echo "OS: '$DEPS_OS_IDENTIFIER' is not in the list of supported platforms!"
        echo
        sleep 2
        return 1
    fi

    local deps_tag=$(get_last_deps_tag)

    # Allow for the dependency tag to be specified (for testing)
    if [ ! -z $FORCED_DEPS_TAG ]; then
        deps_tag=$FORCED_DEPS_TAG
    fi

    local deps_file=$deps_base_name-$deps_tag.tar.gz
    local deps_url=$DEP_URL_BASE/$deps_file

    echo "### Downloading external dependencies from $deps_url. ###"
    echo

    # Make sure curl is available
    check_for_curl
    if [ $? -ne 0 ]; then
        return 1
    fi
    # Make sure conan is available
    check_for_conan
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
    mkdir -p $INSTALL_PREFIX
    mv -f $deps_base_name/* $INSTALL_PREFIX
    rm -rf $deps_base_name

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

install_python() {
    local package_status=$(get_package_manager_status)
    local marker_path=$INSTALL_PREFIX/$DEPS_MANAGER_MARKER_NAME

    element_in "python-install" "${package_status[@]}"
    local vin=$?
    if [ $vin -eq 1 -o "$FORCE" = "y" ]; then
        pushd $scriptDir

        # create default build profile
        safeRunCommandQuiet "conan profile new default --detect --force"

        # python package opts
        local package_opts="./resources/python/conanfile.py cpython/$PYTHON_VERSION@karabo/$CONAN_RECIPE_CHANNEL"
        # configure prefix paths
        local folder_opts="--install-folder=$INSTALL_PREFIX/conan_out --output-folder=$INSTALL_PREFIX"
        # build python if not found in conan cache
        local build_opts="--build=missing -s build_type=Release"
        # apply custom profile on top of default profile
        local profile_opts="-pr:b=./conanprofile.karabo -pr:h=./conanprofile.karabo"
        # always compile patchelf and b2 from source (needed later), avoids CentOS7 failures
        safeRunCommandQuiet "conan install patchelf/0.13@ --build=patchelf $profile_opts"
        safeRunCommandQuiet "conan install b2/4.9.6@ --build=b2 $profile_opts"
        # copy conan recipe from extern/resources/python to local conan cache
        safeRunCommandQuiet "conan export $package_opts"
        # install packages listed in the extern/conanfile-bootstrap.txt
        safeRunCommandQuiet "conan install conanfile-bootstrap.txt $folder_opts $build_opts $profile_opts"

        popd
        echo "python-install" >> $marker_path
    fi
}

download_sources() {
    local package_status=$(get_package_manager_status)
    local marker_path=$INSTALL_PREFIX/$DEPS_MANAGER_MARKER_NAME
    # check for the final step in the package manager based installation
    # if that succeeded we are also done here.
    element_in "pip-requirements" "${package_status[@]}"
    local vin=$?
    if [ $vin -eq 1 -o "$FORCE" = "y" ]; then
        pushd $scriptDir
        safeRunCommand "$INSTALL_PREFIX/bin/python3 -m pip install pyyaml==6.0"
        safeRunCommand "$INSTALL_PREFIX/bin/python3 download_helper.py downloads.yml"
        popd
    fi
}

get_package_manager_status() {
    mkdir -p $INSTALL_PREFIX
    local marker_path=$INSTALL_PREFIX/$DEPS_MANAGER_MARKER_NAME

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

install_from_deps() {
    pushd $scriptDir
    # we use markers to not repeat steps in the build stage.
    # TODO: could create hash digests of the main requirement files
    # and force a rebuild if they changed.
    local package_status=$(get_package_manager_status)
    local marker_path=$INSTALL_PREFIX/$DEPS_MANAGER_MARKER_NAME

    # make sure we have paths set to where conan will place artifacts
    OLD_PATH=$PATH
    OLD_PYTHONPATH=$PYTHONPATH
    OLD_CPATH=$CPATH
    OLD_LD_RUN_PATH=$LD_RUN_PATH
    export PATH=$INSTALL_PREFIX/bin:$PATH
    export PYTHONPATH=$INSTALL_PREFIX/lib/python$PYTHON_PATH_VERSION
    if [ -n "$CPATH" ] ; then
        export CPATH=$INSTALL_PREFIX/include
    else
        export CPATH=$INSTALL_PREFIX/include:$CPATH
    fi
    export LD_RUN_PATH=$INSTALL_PREFIX/lib

    # for the lib paths we need to give the version explicitely
    local python_version=python$PYTHON_PATH_VERSION

    # global pip config
    # we will run pip as a module throughout to avoid errors where a pip import
    # is not found when using the pip3 command directly.
    # These sometimes occur if pip does large changes, as part of dependency
    # chains affecting its own requirements.
    local pip_install_cmd="$INSTALL_PREFIX/bin/pip install"
    local pip_target="--target $INSTALL_PREFIX/lib/$python_version"

    # force a reinstall of pip first, such that it works properly
    element_in "pip-upgrade" "${package_status[@]}"
    local vin=$?
    if [ $vin -eq 1 -o "$FORCE" = "y" ]; then
        safeRunCommand "curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py"
        safeRunCommand "$INSTALL_PREFIX/bin/python get-pip.py --force-reinstall"
        safeRunCommand "rm get-pip.py"
        echo "pip-upgrade" >> $marker_path
    fi

    # install requirements that should be installed before everything else,
    # such as pip, conan, and other tools to build/install packages
    # since this includes conan, we need to force a reinstall, otherwise
    # only the stdlib version of conan will be available, no commands can
    # be issued directly from the CLI.
    element_in "pip-pre-requirements" "${package_status[@]}"
    local vin=$?
    if [ $vin -eq 1 -o "$FORCE" = "y" ]; then
        safeRunCommandQuiet "$pip_install_cmd --force-reinstall -r requirements-pre.txt"
        echo "pip-pre-requirements" >> $marker_path
    fi

    # next is conan
    element_in "conan" "${package_status[@]}"
    local vin=$?
    if [ $vin -eq 1 -o "$FORCE" = "y" ]; then

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
        local folder_opts="--install-folder=$INSTALL_PREFIX/conan_out --output-folder=$INSTALL_PREFIX"
        # when should conan build from sources? missing means if no pre-compiled binary package exists
        # boost:python_executable comes from a variable, so it must be defined here
        local build_opts="--build=missing -o boost:python_executable=$INSTALL_PREFIX/bin/python"
        # apply custom profile on top of default profile
        local profile_opts="-pr:b=./conanprofile.karabo -pr:h=./conanprofile.karabo"

        # install packages listed in the extern/conanfile.txt
        safeRunCommandQuiet "$INSTALL_PREFIX/bin/conan install . $folder_opts $build_opts $profile_opts"

        echo "conan" >> $marker_path
    fi

    # with conan installed, now build shipped dependencies
    # this way python can already pick them up.
    # What have we been asked to build? - this builds the rest
    determine_build_packages

    # Build the dependencies
    build_dependencies

    # install python requirements
    # we do this in multiple stages, as pip has issues resolving a
    # too complex dependency chain with many pinned versions.
    element_in "pip-requirements" "${package_status[@]}"
    local vin=$?
    if [ $vin -eq 1 -o "$FORCE" = "y" ]; then
        # important dependencies that many other packages will need
        # (ie: numpy) are installed first
        safeRunCommandQuiet "$pip_install_cmd -r requirements-0.txt"
        # install everything else
        safeRunCommandQuiet "$pip_install_cmd -r requirements-1.txt"
        echo "pip-requirements" >> $marker_path
    fi

    # check numpy capabilities
    $INSTALL_PREFIX/bin/python -c "import numpy; print(numpy.show_config())"

    # for whatever reason conan does not reliably copy *.pc files from its root directory
    # we do this here instead, and also capture any .pc files our from source builds created
    # in the process.
    safeRunCommand "mkdir -p $INSTALL_PREFIX/lib/pkgconfig/"
    cp $INSTALL_PREFIX/conan_out/*.pc $INSTALL_PREFIX/lib/pkgconfig/
    # now fix occurences of prefixes such that packages can use the "--define-prefix" option
    # of pkgconfig
    safeRunCommand "sed -i 's|prefix=.*|prefix=\${KARABO}/extern|g' $INSTALL_PREFIX/lib/pkgconfig/*.pc"
    safeRunCommand "sed -i 's|libdir=.*|libdir=\${prefix}/lib|g' $INSTALL_PREFIX/lib/pkgconfig/*.pc"
    safeRunCommand "sed -i 's|includedir=.*|includedir=\${prefix}/include|g' $INSTALL_PREFIX/lib/pkgconfig/*.pc"
    safeRunCommand "sed -i 's|exec_prefix=.*|exec_prefix=\${prefix}|g' $INSTALL_PREFIX/lib/pkgconfig/*.pc"

    popd

    # reset the environment
    if [ -n "$OLD_PATH" ] ; then
        export PATH=$OLD_PATH
    fi

    if [ -n "$OLD_PYTHONPATH" ] ; then
        export PYTHONPATH=$OLD_PYTHONPATH
    else
        unset PYTHON_PATH
    fi

    if [ -n "$OLD_CPATH" ] ; then
        export CPATH=$OLD_CPATH
    else
        unset CPATH
    fi

    if [ -n "$OLD_LD_RUN_PATH" ] ; then
        export LD_RUN_PATH=$OLD_LD_RUN_PATH
    else
        unset LD_RUN_PATH
    fi

}

usage() {
    echo
    echo "Build the Karabo Dependencies"
    echo
    echo "Usage: $0 [args] INSTALL_DIRECTORY CI|ALL|PYTHON|DB|<list>"
    echo "  INSTALL_DIRECTORY : The directory where build artifacts are installed"
    echo "  CI|ALL|PYTHON|DB|<list> : The type of build to perform, OR"
    echo "                                      <list> is a list of packages to build"
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

if [ "$WHAT" = "ALL" ]; then
    # Attempt to install dependencies from FTP
    try_dependency_download
fi

# python download and install to allow full bootstrap
install_python

# download any sources we build ourselfs
download_sources

# install via conan and pip next
install_from_deps

# Package everything up, if requested
if [ "$BUILD_PACKAGE" = "y" ]; then
    package_deps_directory
fi

echo "### All external dependencies successfully installed/already present. ###"
