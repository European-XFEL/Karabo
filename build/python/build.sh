#!/bin/bash

# Get the version as a tag name, if one exists
VERSION=$(git describe --exact-match HEAD 2>/dev/null)
if [ $? -ne 0 ]; then
    # Otherwise use the short hash
    VERSION=$(git rev-parse --short HEAD)
fi

if [ -z $KARABO ]; then
    echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
    exit 1
fi


#sed -i "/version/c\    version=\"$VERSION\"," setup.py 
echo $VERSION > VERSION
sed -i s/VERSION/$VERSION/ scripts/win_post_install.py
sed -i s/VERSION/$VERSION/ setup.cfg

#( cd ../../extern/resources/suds; unzip -q suds-jurko-0.6.zip )

$KARABO/extern/bin/python3 ../../src/pythonGui/setup.py bdist_wininst

sed -i s/$VERSION/VERSION/ scripts/win_post_install.py
sed -i s/$VERSION/VERSION/ setup.cfg

