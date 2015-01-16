#!/bin/bash


tmp=$(svn info ../../ | grep URL)
VERSION=${tmp##*/}
if [ "$VERSION" = "trunk" ]; then
    tmp=$(svn info ../../ | grep Revision)
    VERSION=r${tmp##*: }
fi

if [ -z $KARABO ]; then
    if [ -e $HOME/.karabo/karaboFramework ]; then
        KARABO=$(cat $HOME/.karabo/karaboFramework)
    else
      echo "ERROR Could not find karaboFramework. Make sure you have installed the karaboFramework."
      exit 1
    fi
fi


#sed -i "/version/c\    version=\"$VERSION\"," setup.py 
echo $VERSION > VERSION
sed -i s/VERSION/$VERSION/ scripts/win_post_install.py
sed -i s/VERSION/$VERSION/ setup.cfg

#( cd ../../extern/resources/suds; unzip -q suds-jurko-0.6.zip )

$KARABO/extern/bin/python3 setup.py  bdist_wininst

sed -i s/$VERSION/VERSION/ scripts/win_post_install.py
sed -i s/$VERSION/VERSION/ setup.cfg

