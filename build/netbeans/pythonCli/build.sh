#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin
cat > karabo-cli <<End-of-file
#!/bin/bash
#
# This file was automatically generated. Do not edit.
#
if [ -z \$KARABO ]; then
    if [ -e \$HOME/.karabo/karaboFramework ]; then
        KARABO=\$(cat \$HOME/.karabo/karaboFramework)
    else
      echo "ERROR Could not find karaboFramework. Make sure you have installed the karaboFramework."
      exit 1
    fi
fi
export PYTHONPATH=\$KARABO/lib
export PATH=\$KARABO/extern/bin:\$PATH
\$KARABO/extern/bin/ipython.py -i \$KARABO/lib/pythonCli/deviceClient.py \$@
End-of-file
chmod u+x karabo-cli
cd ../lib
cp -rf ../../../../../../src/pythonCli .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
