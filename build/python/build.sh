#!/bin/bash

if [ -z $KARABO ]; then
    echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
    exit 1
fi

PYTHON=$KARABO/extern/bin/python3

pushd ../../src/pythonGui

# Build once with the normal build script to do the heavy lifting
# This also creates VERSION, PACKAGES, and PKGDATA files
$PYTHON setup.py krb_windows_build

VERSION=$(cat VERSION)

#sed -i "/version/c\    version=\"$VERSION\"," setup.py
sed -i s/VERSION/$VERSION/ scripts/win_post_install.py
sed -i s/VERSION/$VERSION/ setup.cfg

#( cd ../../extern/resources/suds; unzip -q suds-jurko-0.6.zip )

$PYTHON scripts/win_setup.py bdist_wininst

sed -i s/$VERSION/VERSION/ scripts/win_post_install.py
sed -i s/$VERSION/VERSION/ setup.cfg

# Clean up
rm VERSION METADATA

popd
