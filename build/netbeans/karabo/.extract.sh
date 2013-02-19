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
echo "                 Karabo - The European XFEL Software Framework"
echo "                              Version: $VERSION"
echo                                            
echo " #####################################################################"
echo
echo " NOTE: This installer will NOT change any settings on your machine."
echo "       Installation will be limited to a directory (\"karabo-$VERSION\")" 
echo "       under the specified path and a $HOME/.karabo for private settings."
echo

installDir=$HOME
if [ "x${install_prefix_dir}x" != "xx" ]
then
  installDir="${install_prefix_dir}"
fi

echo "This is a self-extracting archive."
if [ "x${interactive}x" = "xTRUEx" ]
then
  read -e -p " Installation path [$HOME]: " dir
    # Always reslove to absolute path
    #installDir=`[[ $dir = /* ]] && echo "$dir" || echo "$PWD/${dir#./}"`
    #mkdir -p $installDir
  if [ "x${dir}x" != "xx" ]; then
     if [ ! -d ${dir} ]; then
       mkdir -p  ${dir} ||  echo_exit "Cannot create directory $dir"
     fi
     installDir=`cd "${dir}"; pwd`
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
echo -n "Running post install script..."
mkdir -p $HOME/.karabo
echo $installDir/karabo-$VERSION > $HOME/.karabo/karaboFramework
echo " done."
echo
echo
echo " Karabo was successfully installed to: $installDir/karabo-$VERSION"
echo
exit 0
# NOTE: Don't place any newline characters after the last line below.
__TARFILE_FOLLOWS__
