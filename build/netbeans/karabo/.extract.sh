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
echo "       Installation will be limited to a directory (\"karabo-$VERSION\")" 
echo "       under the specified path and a $HOME/.karabo for private settings."
echo
echo "       In case you also install Karabo's run environment a directory"
echo "       (\"karaboRun\") will be created under the specified run-path."
echo "       Already existing karaboRun folders will be updated."
echo 

installDir=$HOME
runDir=$HOME
if [ "x${install_prefix_dir}x" != "xx" ]; then
    installDir="${install_prefix_dir}"
    runDir=0
fi

echo " This is a self-extracting archive."
if [ "x${interactive}x" = "xTRUEx" ]; then
    read -e -p " Framework installation path [$HOME]: " dir
    dir=${dir/#\~/$HOME}
    # Always resolve to absolute path
    #dir=$(dirname `[[ $dir = /* ]] && echo "$dir" || echo "$PWD/${dir#./}"`)
    #mkdir -p $installDir
    if [ "x${dir}x" != "xx" ]; then
	if [ ! -d ${dir} ]; then
	    mkdir -p  ${dir} ||  echo_exit "Cannot create directory $dir"
	fi
	installDir=`cd "${dir}"; pwd`
    fi
    read -e -p " Run environment installation path (type 0 to skip) [$HOME]: " dir
    dir=${dir/#\~/$HOME}
    # Always resolve to absolute path
    #dir=$(dirname `[[ $dir = /* ]] && echo "$dir" || echo "$PWD/${dir#./}"`)
    if [ "$dir" = "0" ]; then 
	runDir=0
    elif [ "x${dir}x" != "xx" ]; then
	if [ ! -d ${dir} ]; then
	    mkdir -p  ${dir} ||  echo_exit "Cannot create directory $dir"
	fi
	runDir=`cd "${dir}"; pwd`
	
    fi
fi
echo -n " Extracting files, please wait..."
# searches for the line number where finish the script and start the tar.gz
SKIP=`awk '/^__TARFILE_FOLLOWS__/ { print NR + 1; exit 0; }' $0`
#remember our file name
#THIS=`pwd`/$0
# take the tarfile and pipe it into tar
#cd $installDir
#tail -n +$SKIP $THIS | tar -xz
tail -n +$SKIP $0 | (cd  $installDir && tar xzf -) || echo_exit "Problem unpacking the file $0"
echo  " unpacking finished successfully"
# Any script here will happen after the tar file extract.
echo
echo -n " Running post install script..."
if [ "$runDir" != "0" ]; then
    cp -rf $installDir/karabo-$VERSION/karaboRun $runDir/
fi
mkdir -p $HOME/.karabo
echo $installDir/karabo-$VERSION > $HOME/.karabo/karaboFramework
#echo "https://svnsrv.desy.de/desy/EuXFEL/WP76/karabo" > $HOME/.karabo/karaboSvnPath
# change sipconfig.py
#
# find site-packages directory using installed python
sitePackagesDir=`$installDir/karabo-$VERSION/extern/bin/python -c "import site; print(site.getsitepackages()[0])"`
includeDir=`$installDir/karabo-$VERSION/extern/bin/python3-config --includes`
configDir=`$installDir/karabo-$VERSION/extern/bin/python3-config --configdir`
cd $sitePackagesDir
sitePackagesDir=${sitePackagesDir##*/extern/}
includeDir=${includeDir##*/extern/}
configDir=${configDir##*/extern/}
sed -i "/'default_bin_dir'/c\    'default_bin_dir':    '`cat $HOME/.karabo/karaboFramework`/extern/bin'," sipconfig.py
sed -i "/'default_mod_dir'/c\    'default_mod_dir':    '`cat $HOME/.karabo/karaboFramework`/extern/$sitePackagesDir'," sipconfig.py
sed -i "/'default_sip_dir'/c\    'default_sip_dir':    '`cat $HOME/.karabo/karaboFramework`/extern/share/sip'," sipconfig.py
sed -i "/'py_conf_inc_dir'/c\    'py_conf_inc_dir':    '`cat $HOME/.karabo/karaboFramework`/extern/$includeDir'," sipconfig.py
sed -i "/'py_inc_dir'/c\    'py_inc_dir':         '`cat $HOME/.karabo/karaboFramework`/extern/$includeDir'," sipconfig.py
sed -i "/'py_lib_dir'/c\    'py_lib_dir':         '`cat $HOME/.karabo/karaboFramework`/extern/$configDir'," sipconfig.py
sed -i "/'sip_bin'/c\    'sip_bin':            '`cat $HOME/.karabo/karaboFramework`/extern/bin/sip'," sipconfig.py
sed -i "/'sip_inc_dir'/c\    'sip_inc_dir':        '`cat $HOME/.karabo/karaboFramework`/extern/$includeDir'," sipconfig.py
sed -i "/'sip_mod_dir'/c\    'sip_mod_dir':        '`cat $HOME/.karabo/karaboFramework`/extern/$sitePackagesDir'," sipconfig.py
cd -
echo " done."
echo
echo
echo " Karabo framework was successfully installed to: $installDir/karabo-$VERSION"
echo
if [ "$runDir" != "0" ]; then
    echo " Karabo's run environment was successfully installed to: $runDir/karaboRun"
else
    echo " Karabo's run environment was NOT installed."
fi
echo
exit 0
# NOTE: Don't place any newline characters after the last line below.
__TARFILE_FOLLOWS__
