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
read -e -p " Installation path [$HOME]: " dir
echo
installDir=$HOME
if [ "$dir" != "" ]; then
    # Always reslove to absolute path
    installDir=`[[ $dir = /* ]] && echo "$dir" || echo "$PWD/${dir#./}"`
    mkdir -p $installDir
fi
echo -n " Extracting files, please wait..."
# searches for the line number where finish the script and start the tar.gz
SKIP=`awk '/^__TARFILE_FOLLOWS__/ { print NR + 1; exit 0; }' $0`
#remember our file name
THIS=`pwd`/$0
# take the tarfile and pipe it into tar
cd $installDir
tail -n +$SKIP $THIS | tar -xz
# Any script here will happen after the tar file extract.
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
