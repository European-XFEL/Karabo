
echo_exit()
{
    echo $1
    exit 1
}

if [ -z $KARABO ]; then
    echo_exit "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to target."
else
    LOCALKARABOVERSION=$(cat $KARABO/VERSION)
    if [ "$LOCALKARABOVERSION" != "$KARABOVERSION" ]; then
        echo "Plugin was compiled with different karaboFramework version"
        echo "than installed one:"
        echo "Compiled: $KARABOVERSION vs. Installed: $LOCALKARABOVERSION"
        echo " "

        read -e -p " Continue installation? [Y/n] " RESPONSE
        if [ "$RESPONSE" == "N" || "$RESPONSE" == "n" ]; then
            exit 1
        fi
    fi
fi

echo "This is a self-extracting archive."
echo -n " Extracting files, please wait..."
# searches for the line number where finish the script and start the .whl
SKIP=`awk '/^__WHEELFILE_FOLLOWS__/ { print NR + 1; exit 0; }' $0`
tail -n +$SKIP $0 | cat - > $WHEELNAME || echo_exit "Problem unpacking the file $0"
echo  " unpacking finished successfully"

echo
echo -n "Running wheel installation..."

OS=$(uname -s)
if [ "$OS" == "Darwin" ]; then
    PIP=/opt/local/bin/pip
    WHEEL_INSTALL_FLAGS="--user"
else
    PIP=$KARABO/extern/bin/pip
    WHEEL_INSTALL_FLAGS=
fi

$PIP install -U --no-index $WHEEL_INSTALL_FLAGS $WHEELNAME

echo " done."
echo
echo " Package was successfully installed to: $KARABO"
echo
echo

# Clean up
rm $WHEELNAME

exit 0
# NOTE: Don't place any newline characters after the last line below.
__WHEELFILE_FOLLOWS__
