#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin
cat > karabo-gui <<End-of-file
#
# This file was automatically generated. Do not edit.
#
CWD=\$(pwd)
cd \$(dirname \$0)/../lib/pythonGui
export PYTHONPATH=../
../../extern/bin/python karabo-gui.py
cd \$CWD
End-of-file
chmod u+x karabo-gui
cd ../lib
cp -rf ../../../../../../src/pythonGui .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
