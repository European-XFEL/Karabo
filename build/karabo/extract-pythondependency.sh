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
PIP=$KARABO/extern/bin/pip
WHEEL_INSTALL_FLAGS=

$PIP --disable-pip-version-check install -U --no-index $WHEEL_INSTALL_FLAGS $WHEELNAME
retVal=$?
if [ $retVal -ne 0 ]; then
    echo "ERROR: pip returned $retval. Exiting."
    exit $retVal
fi

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
