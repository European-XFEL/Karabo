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
echo "                       Karabo plugin: $PLUGINNAME"
echo "       version: $VERSION, karaboFramework version: $KARABOVERSION"
echo                                            
echo " #####################################################################"
echo
installDir=$HOME/packages
if [ -z $KARABO ]; then
    if [ -e $HOME/.karabo/karaboFramework ]; then
        KARABO=$(cat $HOME/.karabo/karaboFramework)
        LOCALKARABOVERSION=$(cat $KARABO/VERSION)
        if [ "$LOCALKARABOVERSION" == "$KARABOVERSION" ]; then
           installDir=$KARABO/plugins
        else
           echo "Plugin was compiled with different karaboFramework version"
           echo "than installed one: $KARABOVERSION vs. $LOCALKARABOVERSION"
           echo " "
        fi
    else
      echo "ERROR Could not find karaboFramework. Make sure you have installed the karaboFramework"
      echo "and create links to installed plugins"
    fi
else
    LOCALKARABOVERSION=$(cat $KARABO/VERSION)
    if [ "$LOCALKARABOVERSION" == "$KARABOVERSION" ]; then
       installDir=$KARABO/plugins
    else
       echo "Plugin was compiled with different karaboFramework version"
       echo "than installed one: $KARABOVERSION vs. $LOCALKARABOVERSION"
       echo " "
    fi
fi

if [ "x${install_prefix_dir}x" != "xx" ]
then
  installDir="${install_prefix_dir}"
fi

echo "This is a self-extracting archive."
if [ "x${interactive}x" = "xTRUEx" ]
then
  read -e -p " Installation path [$installDir]: " dir
    # Always resolve to absolute path
    #installDir=`[[ $dir = /* ]] && echo "$dir" || echo "$PWD/${dir#./}"`
    #mkdir -p $installDir
  if [ "x${dir}x" != "xx" ]; then
     if [ ! -d ${dir} ]; then
       mkdir -p  ${dir} ||  echo_exit "Cannot create directory $dir"
     fi
     installDir=`cd "${dir}"; pwd`
  else
     if [ ! -d ${installDir} ]; then
       mkdir -p  ${installDir} ||  echo_exit "Cannot create directory $installDir"
     fi
  fi
fi

echo -n " Extracting files, please wait..."
# searches for the line number where finish the script and start the tar.gz
SKIP=`awk '/^__TARFILE_FOLLOWS__/ { print NR + 1; exit 0; }' $0`
tail -n +$SKIP $0 | (cd  $installDir && tar xzf -) || echo_exit "Problem unpacking the file $0"
echo  " unpacking finished successfully"
# Any script here will happen after the tar file extract.
echo
echo -n "Running post install script..."
#if [ "x${KARABO}x" != "xx" ]; then
#   if [ ! -d ${KARABO}/pythonplugins ]; then
#      mkdir ${KARABO}/pythonplugins || echo_exit "Cannot create directory ${KARABO}/pythonplugins"
#   fi
#   echo "Creating link to plugins folder..."
#   #olddir=`pwd`; cd ${KARABO}/plugins; ln -sf $installDir/$PLUGINNAME-$VERSION/*; cd $olddir
#   olddir=`pwd`; cd ${KARABO}/pythonplugins; find $installDir/$PLUGINNAME-$VERSION -type f -exec ln -sf {} \; ; cd $olddir
#fi
echo " done."
echo
echo " Package was successfully installed to: $installDir/$PLUGINNAME-$VERSION-$KARABOVERSION"
echo
echo
exit 0
# NOTE: Don't place any newline characters after the last line below.
__TARFILE_FOLLOWS__
