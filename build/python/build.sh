#!/bin/bash


tmp=$(svn info ../../ | grep URL)
VERSION=${tmp##*/}
if [ "$VERSION" = "trunk" ]; then
    tmp=$(svn info ../../ | grep Revision)
    VERSION=r${tmp##*: }
fi


#sed -i "/version/c\    version=\"$VERSION\"," setup.py 
echo $VERSION > VERSION

( cd ../../extern/resources/suds; unzip -q suds-jurko-0.6.zip )

python setup.py  bdist_wininst


