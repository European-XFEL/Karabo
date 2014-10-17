#!/bin/bash


tmp=$(svn info ../../ | grep URL)
VERSION=${tmp##*/}
if [ "$VERSION" = "trunk" ]; then
    tmp=$(svn info ../../ | grep Revision)
    VERSION=r${tmp##*: }
fi


#sed -i "/version/c\    version=\"$VERSION\"," setup.py 
echo $VERSION > VERSION
ln -s ../../src/pythonGui
ln -s ../../src/pythonKarabo
( cd ../../extern/resources/suds; unzip -q -o suds-jurko-0.5.zip )
ln -s ../../extern/resources/suds/suds-jurko-0.5/suds

python setup.py  sdist


