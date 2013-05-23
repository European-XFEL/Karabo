#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin
cat > karabo-gui2 <<End-of-file
#
# This file was automatically generated. Do not edit.
#
# Author: <burkhard.heisen@xfel.eu>
#
CWD=\$(pwd)
cd \$(dirname \$0)/../lib/pythonGui2
export PYTHONPATH=../
export LD_LIBRARY_PATH=../../extern/lib
export DYLD_LIBRARY_PATH=../../extern/lib
export PATH=../../extern/bin
python karabo-gui.py
cd \$CWD

End-of-file
chmod u+x karabo-gui2
cd ../lib
cp -rf ../../../../../../src/pythonGui2 .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
