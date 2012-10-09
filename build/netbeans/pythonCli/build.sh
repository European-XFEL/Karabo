#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin
cat > karabo-cli <<End-of-file
#
# This file was automatically generated. Do not edit.
#
CWD=\$(pwd)
cd \$(dirname \$0)/../lib/pythonCli
export PYTHONPATH=../
../../extern/bin/ipython -i deviceClient.py
cd \$CWD
End-of-file
chmod u+x karabo-cli
cd ../lib
cp -rf ../../../../../../src/pythonCli .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
