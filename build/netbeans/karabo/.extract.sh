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
	upgrade="y"
	read -e -p "       Continue? [Y/n]: " upgrade
	if [ "x${upgrade}x" != "xyx" -a "x${upgrade}x" != "xYx" ]; then
	    echo
	    echo " Installation aborted."
	    echo
	    exit 0
	fi	
    fi
fi



echo -n " Extracting files, please wait..."
# searches for the line number where finish the script and start the tar.gz
SKIP=`awk '/^__TARFILE_FOLLOWS__/ { print NR + 1; exit 0; }' $0`
tail -n +$SKIP $0 | (cd  $installDir && tar xzf -) || echo_exit "Problem unpacking the file $0"
echo  " unpacking finished successfully"
# Any script here will happen after the tar file extract.

# Fix up the venv activation script
KARABO=$(get_abs_path $installDir/karabo)
sed "s%__VENV_DIR__%$KARABO%g" $KARABO/bin/activate.tmpl > $KARABO/activate

# Make sure the ~/.karabo directory exists
mkdir -p $HOME/.karabo

# fix the shebang line of Python entry-points
safeRunCommand "$KARABO/bin/.fix-python-scripts.sh" $KARABO

# add the config file to var/config in case not existing yet
if [ ! -e "$KARABO/var/config/config" ]; then    
    cp $KARABO/bin/config.orig $KARABO/var/config/config
fi

# change sipconfig.py
#
# find site-packages directory using installed python
sitePackagesDir=`$KARABO/extern/bin/python -c "import site; print(site.getsitepackages()[0])"`
includeDir=`$KARABO/extern/bin/python3-config --includes`
configDir=`$KARABO/extern/bin/python3-config --configdir`
cd $sitePackagesDir
sitePackagesDir=${sitePackagesDir##*/extern/}
includeDir=${includeDir##*/extern/}
configDir=${configDir##*/extern/}
sed -i "/'default_bin_dir'/c\    'default_bin_dir':    '$KARABO/extern/bin'," sipconfig.py
sed -i "/'default_mod_dir'/c\    'default_mod_dir':    '$KARABO/extern/$sitePackagesDir'," sipconfig.py
sed -i "/'default_sip_dir'/c\    'default_sip_dir':    '$KARABO/extern/share/sip'," sipconfig.py
sed -i "/'py_conf_inc_dir'/c\    'py_conf_inc_dir':    '$KARABO/extern/$includeDir'," sipconfig.py
sed -i "/'py_inc_dir'/c\    'py_inc_dir':         '$KARABO/extern/$includeDir'," sipconfig.py
sed -i "/'py_lib_dir'/c\    'py_lib_dir':         '$KARABO/extern/$configDir'," sipconfig.py
sed -i "/'sip_bin'/c\    'sip_bin':            '$KARABO/extern/bin/sip'," sipconfig.py
sed -i "/'sip_inc_dir'/c\    'sip_inc_dir':        '$KARABO/extern/$includeDir'," sipconfig.py
sed -i "/'sip_mod_dir'/c\    'sip_mod_dir':        '$KARABO/extern/$sitePackagesDir'," sipconfig.py
cd - >/dev/null
echo
echo " Karabo framework was successfully installed to: $KARABO"
echo 
echo " NOTE: Using Karabo requires you to source the 'activate' file, like so:"
echo
echo "       source $KARABO/activate"
echo
exit 0
# NOTE: Don't place any newline characters after the last line below.
__TARFILE_FOLLOWS__
