# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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

    eval $cmnd
    ret_code=$?
    if [ $ret_code != 0 ]; then
        printf "Error : [%d] when executing command: '$cmnd'" $ret_code
        exit $ret_code
    fi
}

usage()
{
  cat <<EOF
Usage: $0 [options]
Options:
  --help       print this message
  --prefix=dir install directory
EOF
  exit 1
}

echo_exit()
{
  echo $1
  exit 1
}

interactive=TRUE
install_prefix_dir=""
for a in $@; do
  if echo $a | grep "^--prefix=" > /dev/null 2> /dev/null; then
     install_prefix_dir=`echo $a | sed "s/^--prefix=//"`
     install_prefix_dir=${install_prefix_dir/#\~/$HOME}
     if [ ! -d ${install_prefix_dir} ]; then
       mkdir -p  ${install_prefix_dir} ||  echo_exit "Cannot create directory ${install_prefix_dir}"
     fi
     install_prefix_dir=`cd "${install_prefix_dir}"; pwd`
  fi
  if echo $a | grep "^--help" > /dev/null 2> /dev/null; then
    usage 
  fi
done

if [ "x${install_prefix_dir}x" != "xx" ]; then
  interactive=FALSE
fi


echo
echo " #####################################################################"
echo "                                INSTALLATION"
echo                                           
echo "                 Karabo - The European XFEL Software Framework"
echo "                              Version: $VERSION"
echo                                            
echo " #####################################################################"
echo
echo " NOTE: This installer will NOT change any settings on your machine."
echo "       Installation will be limited to a directory (\"karabo\")" 
echo "       under the specified path and a $HOME/.karabo for private settings."
echo
echo 

installDir=$HOME
runDir=$HOME
upgrade="n"
if [ "x${install_prefix_dir}x" != "xx" ]; then
    installDir="${install_prefix_dir}"
    runDir=0
fi

if [ "x${interactive}x" = "xTRUEx" ]; then
    read -e -p " Karabo installation path [$HOME]: " dir
    dir=${dir/#\~/$HOME}
    # Always resolve to absolute path
    if [ "x${dir}x" != "xx" ]; then
	if [ ! -d ${dir} ]; then
	    mkdir -p  ${dir} ||  echo_exit "Cannot create directory $dir"
	fi
	installDir=`cd "${dir}"; pwd`
    fi
    # Check for any previous karabo installation
    if [ -e "$installDir/karabo/VERSION" ]; then
	echo 
	echo " WARN: Found an existing Karabo installation ($(cat $installDir/karabo/VERSION))"
	echo "       Continuing will upgrade the core," 
	echo "       but leave config, log and history files untouched."
	read -e -p "       Continue? [Y/n]: " -i y upgrade
	if [ "x${upgrade}x" != "xyx" -a "x${upgrade}x" != "xYx" ]; then
	    echo
	    echo " Installation aborted."
	    echo
	    exit 0
	else
	    for DIRECTORY in bin extern include lib plugins
	    do
	        dirDelete=$installDir/karabo/$DIRECTORY
	        if [ -d $dirDelete ]; then
	        	echo " Removing karabo/$DIRECTORY.."
	            rm -rf $dirDelete
	        fi
	    done
	fi
    fi
fi

echo -n " Extracting files, please wait..."
unzip -d "$installDir" -qo "$0"
echo  " unpacking finished successfully"
# Any script here will happen after the zip file extract.

# Fix up the venv activation script
KARABO=$(get_abs_path $installDir/karabo)
sed "s%__VENV_DIR__%$KARABO%g" $KARABO/bin/activate.tmpl > $KARABO/activate

for SUBPATH in lib lib/pkgconfig; do
    karabo_pkg_conf_file=$KARABO/$SUBPATH/karaboDependencies.pc
    if [ -f ${karabo_pkg_conf_file} ]; then
        # Inject the right install prefix needed for karabo cpp library
        sed -i s%^prefix=.*%prefix=$KARABO%g ${karabo_pkg_conf_file}
    fi
done

# Make sure the ~/.karabo directory exists
mkdir -p $HOME/.karabo

# fix the shebang line of Python entry-points
safeRunCommand "$KARABO/bin/.fix-python-scripts.sh" $KARABO

# Create some directories
# 1. var/log (for supervisor)
mkdir -p $KARABO/var/log
# 2. var/data (as home directory of all server processes)
mkdir -p $KARABO/var/data
# 3. plugins (existence assumed e.g. by karabo script)
mkdir -p $KARABO/plugins
# 4. var/data/env_scripts as placeholder for custom activate scripts
mkdir -p $KARABO/var/data/env_scripts
echo "On Karabo's ``activate`` and ``deactivate`` actions" > $KARABO/var/data/env_scripts/README.md
echo "the scripts ``activate.sh`` and a ``deactivate.sh``" >> $KARABO/var/data/env_scripts/README.md
echo "in this folder will be sourced respectively if present." >> $KARABO/var/data/env_scripts/README.md

echo
echo " Karabo framework was successfully installed to: $KARABO"
echo
echo " NOTE: Using Karabo requires you to source the 'activate' file, like so:"
echo
echo "       source $KARABO/activate"
echo
exit 0
